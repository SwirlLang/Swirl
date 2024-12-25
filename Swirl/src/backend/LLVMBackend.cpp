#include <string>
#include <ranges>
#include <unordered_map>

#include <parser/Parser.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/Casting.h>
#include <llvm/IR/Verifier.h>

// ReSharper disable all CppUseStructuredBinding

extern std::string SW_OUTPUT;

class NewScope {
    bool prev_ls_cache{};
    LLVMBackend& instance;

public:
     explicit NewScope(LLVMBackend& inst): instance(inst) {
         prev_ls_cache = inst.IsLocalScope;
         inst.IsLocalScope = true;
         inst.SymManager.newScope();
     }

    ~NewScope() {
         instance.IsLocalScope = prev_ls_cache;
         instance.SymManager.destroyLastScope();
         instance.ChildHasReturned = false;
     }
};


void codegenChildrenUntilRet(LLVMBackend& instance, std::vector<std::unique_ptr<Node>>& children) {
    for (const auto& child : children) {
        if (child->getNodeType() == ND_RET) {
            child->llvmCodegen(instance);
            instance.ChildHasReturned = true;
            return;
        } child->llvmCodegen(instance);
    }
}


llvm::Value* IntLit::llvmCodegen(LLVMBackend& instance) {
    llvm::Value* ret;
    if (value.starts_with("0x"))
        ret = llvm::ConstantInt::get(instance.IntegralTypeState, value.substr(2), 16);  // edge-case of "0x" unhandled
    else if (value.starts_with("0o"))
        ret = llvm::ConstantInt::get(instance.IntegralTypeState, value.substr(2), 8);  // edge-case of "0o" unhandled
    else ret = llvm::ConstantInt::get(instance.IntegralTypeState, value, 10);

    // TODO: handle octal and hexal radices
    if (instance.LatestBoundIsFloating) {
        ret = llvm::ConstantFP::get(instance.FloatTypeState, value);
    }
    return ret;
}

llvm::Value* FloatLit::llvmCodegen(LLVMBackend& instance) {
    return llvm::ConstantFP::get(instance.FloatTypeState, value);
}

llvm::Value* StrLit::llvmCodegen(LLVMBackend& instance) {
    return llvm::ConstantDataArray::getString(instance.Context, value, false);
}

llvm::Value* Ident::llvmCodegen(LLVMBackend& instance) {
    const auto e = instance.SymManager.lookupDecl(this->value);
    if (e.is_param) { return e.ptr; }
    if (instance.IsAssignmentLHS) { return e.ptr; }
    return instance.Builder.CreateLoad(e.swirl_type->llvmCodegen(instance), e.ptr);
}

llvm::Value* Function::llvmCodegen(LLVMBackend& instance) {
    const auto fn_sw_type = dynamic_cast<FunctionType*>(instance.SymManager.lookupType(ident));

    auto*               fn_type  = llvm::dyn_cast<llvm::FunctionType>(fn_sw_type->llvmCodegen(instance));
    llvm::Function*     func     = llvm::Function::Create(fn_type, llvm::GlobalValue::InternalLinkage, ident->toString(), instance.LModule.get());
    llvm::BasicBlock*   entry_bb = llvm::BasicBlock::Create(instance.Context, "entry", func);

    NewScope _(instance);
    instance.Builder.SetInsertPoint(entry_bb);

    for (int i = 0; i < params.size(); i++) {
        const auto p_name = params[i].var_ident;
        const auto param = func->getArg(i);
        param->setName(p_name->toString());

        instance.SymManager.lookupDecl(p_name).ptr = func->getArg(i);
    }

    // for (const auto& child : this->children)
    //     child->codegen();

    codegenChildrenUntilRet(instance, children);
    if (!instance.Builder.GetInsertBlock()->back().isTerminator())
        instance.Builder.CreateRetVoid();
    return func;
}


llvm::Value* ReturnStatement::llvmCodegen(LLVMBackend& instance) {
    if (!value.expr.empty()) {
        const auto ret = value.llvmCodegen(instance);
        instance.Builder.CreateRet(ret);
        return nullptr;
    }

    instance.Builder.CreateRetVoid();
    return nullptr;
}


llvm::Value* Op::llvmCodegen(LLVMBackend& instance) {
    using SwNode = std::unique_ptr<Node>;
    using NodesVec = std::vector<SwNode>;


    // Maps {operator, arity} to the corresponding operator 'function'
    static std::unordered_map<std::pair<std::string, uint8_t>, std::function<llvm::Value*(NodesVec&)>>
    OpTable = {
        {{"+", 1}, [&instance](const NodesVec& operands) -> llvm::Value* {
            return operands.back()->llvmCodegen(instance);
        }},

        {{"-", 1}, [&instance](const NodesVec& operands) -> llvm::Value* {
            return instance.Builder.CreateNeg(operands.back()->llvmCodegen(instance));
        }},

        {{"+", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            return instance.Builder.CreateAdd(operands.at(0)->llvmCodegen(instance), operands.at(1)->llvmCodegen(instance));
        }},

        {{"-", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            return instance.Builder.CreateSub(operands.at(0)->llvmCodegen(instance), operands.at(1)->llvmCodegen(instance));
        }},

        {{"*", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            return instance.Builder.CreateMul(operands.at(0)->llvmCodegen(instance), operands.at(1)->llvmCodegen(instance));
        }},

        {{"*", 1}, [&instance](const NodesVec& operands) -> llvm::Value* {
            const auto entry = instance.SymManager.lookupDecl(operands.at(0)->getIdentInfo());
            if (entry.is_param)
                return entry.ptr;
            return instance.Builder.CreateLoad(entry.ptr->getType(), entry.ptr);
        }},

        {{"&", 1}, [&instance](const NodesVec& operands) -> llvm::Value* {
            auto lookup = instance.SymManager.lookupDecl(operands.at(0)->getIdentInfo());
            return lookup.ptr;
        }},

        {{"/", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            const auto op1 = operands.at(0)->llvmCodegen(instance);
            const auto op2 = operands.at(1)->llvmCodegen(instance);

            if (instance.LatestBoundIsFloating) {
                return instance.Builder.CreateFDiv(op1, op2);
            } {
                if (instance.IntegralTypeState->isSingleValueType())
                    return instance.Builder.CreateSDiv(op1, op2);
                return instance.Builder.CreateUDiv(op1, op2);
            }
        }},


        {{"==", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            auto op1 = operands.at(0)->llvmCodegen(instance);
            auto op2 = operands.at(1)->llvmCodegen(instance);

            llvm::Type* type1 = op1->getType();
            llvm::Type* type2 = op2->getType();

            if (type1->isFloatingPointTy() || type2->isFloatingPointTy()) {
                if (type1 != type2) {
                    if (type1->getPrimitiveSizeInBits() < type2->getPrimitiveSizeInBits()) {
                        op1 = instance.Builder.CreateFPExt(op1, type2);
                    } else {
                        op2 = instance.Builder.CreateFPExt(op2, type1);
                    }
                } return instance.Builder.CreateFCmpOEQ(op1, op2);
            }

            if (type1->isIntegerTy() && type2->isIntegerTy()) {
                const auto i_size1 = type1->getIntegerBitWidth();
                const auto i_size2 = type2->getIntegerBitWidth();

                if (i_size1 != i_size2) {
                    if (i_size1 < i_size2) {
                        op1 = instance.Builder.CreateZExtOrBitCast(op1, type2);
                    } else {
                        op2 = instance.Builder.CreateZExtOrBitCast(op2, type1);
                    }
                } return instance.Builder.CreateICmpEQ(op1, op2);
            }

            if (type1->isFloatingPointTy() && type2->isIntegerTy()) {
                op2 = instance.Builder.CreateSIToFP(op2, type1);
                return instance.Builder.CreateFCmpOEQ(op1, op2);
            }

            if (type1->isIntegerTy() && type2->isFloatingPointTy()) {
                op1 = instance.Builder.CreateSIToFP(op1, type2);
                return instance.Builder.CreateFCmpOEQ(op1, op2);
            }

            throw std::runtime_error("Unsupported types for equality comparison");
        }},

        {{".", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            static Type* BoundType;  // used to propagate the bound type up the recursion hierarchy
            static int   recursion_depth = 0;

            if (operands.front()->getNodeType() == ND_OP && dynamic_cast<Op*>(operands.front().get())->value == ".") {
                recursion_depth++;
                llvm::Value* op1 = OpTable[{".", 2}](operands.front()->getMutOperands());
                
                // auto struct_t = instance.SymManager.lookupType(BoundType->getIdent());  // dependent struct type
                // auto* struct_type = struct_t->llvmCodegen(instance);

                auto struct_entry   = instance.SymManager.lookupType(BoundType->getIdent());
                auto* struct_swtype = dynamic_cast<StructType*>(struct_entry);

                auto [index, bound_type] = struct_swtype->fields.at(operands.at(1)->getIdentInfo());
                llvm::Type* mem_type = bound_type->llvmCodegen(instance);

                const auto field_ptr = instance.Builder.CreateStructGEP(
                    struct_swtype->llvmCodegen(instance),
                    op1,
                    index
                );

                if (instance.IsAssignmentLHS)
                    return field_ptr;
                return instance.Builder.CreateLoad(mem_type, field_ptr);
            }

            const TableEntry op1_symbol = instance.SymManager.lookupDecl(operands.at(0)->getIdentInfo());
            Type* struct_entry  = op1_symbol.swirl_type;

            auto* struct_t = dynamic_cast<StructType*>(struct_entry);

            auto [index, mem_bound_type] = struct_t->fields.at(operands.at(1)->getIdentInfo());
            llvm::Type* member_ty = mem_bound_type->llvmCodegen(instance);
            BoundType = mem_bound_type;


            const auto field_ptr = instance.Builder.CreateStructGEP(
                struct_t->llvmCodegen(instance),
                operands.front()->llvmCodegen(instance),
                index
                );

            if (instance.IsAssignmentLHS)
                return field_ptr;
            return instance.Builder.CreateLoad(member_ty, field_ptr);
        }},

        {{"||", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            return instance.Builder.CreateLogicalOr(operands.at(0)->llvmCodegen(instance), operands.at(1)->llvmCodegen(instance));
        }}
        // {{">", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
        // }}
    };


    return OpTable[{this->value, arity}](operands);
}



llvm::Value* Expression::llvmCodegen(LLVMBackend& instance) {
    llvm::Type* e_type = nullptr;

    // ReSharper disable once CppDFANullDereference
    e_type = expr_type->llvmCodegen(instance);
    if (e_type->isIntegerTy() && e_type != llvm::Type::getInt1Ty(instance.Context))
        instance.setIntegralTypeState(e_type);
    else if (e_type->isFloatingPointTy())
        instance.setFloatTypeState(e_type);

    const auto val = expr.back()->llvmCodegen(instance);

    // ReSharper disable once CppDFANullDereference
    if (e_type->isIntegerTy() && e_type != llvm::Type::getInt1Ty(instance.Context))
        instance.restoreIntegralTypeState();
    else if (e_type->isFloatingPointTy())
        instance.restoreFloatTypeState();

    return val;
}


llvm::Value* Assignment::llvmCodegen(LLVMBackend& instance) {
    instance.IsAssignmentLHS = true;
    const auto lv = l_value.llvmCodegen(instance);
    instance.IsAssignmentLHS = false;

    instance.Builder.CreateStore(r_value.llvmCodegen(instance), lv);
    return nullptr;
}


llvm::Value* Condition::llvmCodegen(LLVMBackend& instance) {
    const auto parent      = instance.Builder.GetInsertBlock()->getParent();
    const auto if_block    = llvm::BasicBlock::Create(instance.Context, "if", parent);
    const auto else_block  = llvm::BasicBlock::Create(instance.Context, "else", parent);
    const auto merge_block = llvm::BasicBlock::Create(instance.Context, "merge", parent);

    const auto if_cond = bool_expr.llvmCodegen(instance);
    instance.Builder.CreateCondBr(if_cond, if_block, else_block);

    {
        NewScope if_scp{instance};
        instance.Builder.SetInsertPoint(if_block);
        codegenChildrenUntilRet(instance, if_children);

        if (!instance.Builder.GetInsertBlock()->back().isTerminator())
            instance.Builder.CreateBr(merge_block);
    }

    {
        NewScope else_scp{instance};
        instance.Builder.SetInsertPoint(else_block);

        for (auto& [condition, children] : elif_children) {
            NewScope elif_scp{instance};
            const auto cnd = condition.llvmCodegen(instance);

            auto elif_block = llvm::BasicBlock::Create(instance.Context, "elif", parent);
            auto next_elif  = llvm::BasicBlock::Create(instance.Context, "next_elif", parent);

            instance.Builder.CreateCondBr(cnd, elif_block, next_elif);
            instance.Builder.SetInsertPoint(elif_block);
            codegenChildrenUntilRet(instance, children);
            if (!instance.Builder.GetInsertBlock()->back().isTerminator())
                instance.Builder.CreateBr(merge_block);
            instance.Builder.SetInsertPoint(next_elif);
        }

        codegenChildrenUntilRet(instance, else_children);
        if (!instance.Builder.GetInsertBlock()->back().isTerminator())
            instance.Builder.CreateBr(merge_block);
        instance.Builder.SetInsertPoint(merge_block);
    }

    return nullptr;
}

// useless
llvm::Value* Struct::llvmCodegen(LLVMBackend& instance) {
    // std::vector<llvm::Type*> types;
    // std::vector<std::string> names;
    // std::vector<Type*> type_names;
    //
    // types.reserve(members.size());
    // names.reserve(members.size());
    // type_names.reserve(members.size());
    //
    // for (const auto& mem : members)
    //     if (mem->getNodeType() == ND_VAR) {
    //         const auto field = dynamic_cast<Var*>(mem.get());
    //         types.push_back(instance.SymManager.lookupType(field->var_type->getIdent())->llvmCodegen(instance));
    //         names.push_back(field->var_ident->toString());
    //         type_names.push_back(field->var_type);
    //     }
    //
    // auto* struct_ = llvm::StructType::get(instance.Context);
    // struct_->setName(ident->toString());
    // struct_->setBody(types);

    return nullptr;
}


llvm::Value* WhileLoop::llvmCodegen(LLVMBackend& instance) {
    const auto parent = instance.Builder.GetInsertBlock()->getParent();
    const auto last_inst = instance.Builder.GetInsertBlock()->getTerminator();

    const auto cond_block  = llvm::BasicBlock::Create(instance.Context, "while_cond", parent);
    const auto body_block  = llvm::BasicBlock::Create(instance.Context, "while_body", parent);
    const auto merge_block = llvm::BasicBlock::Create(instance.Context, "merge", parent);

    if (last_inst == nullptr)
        instance.Builder.CreateBr(cond_block);

    instance.Builder.SetInsertPoint(cond_block);
    const auto expr  = condition.llvmCodegen(instance);
    instance.Builder.CreateCondBr(expr, body_block, merge_block);

    {
        NewScope _(instance);
        instance.Builder.SetInsertPoint(body_block);
        codegenChildrenUntilRet(instance, children);
        instance.Builder.CreateBr(cond_block);
    }

    instance.Builder.SetInsertPoint(merge_block);
    return nullptr;
}


llvm::Value* FuncCall::llvmCodegen(LLVMBackend& instance) {
    std::vector<llvm::Type*> paramTypes;

    llvm::Function* func = instance.LModule->getFunction(ident->toString());
    if (!func)
        throw std::runtime_error("No such function defined");

    std::vector<llvm::Value*> arguments{};
    arguments.reserve(args.size());

    for (auto& item : args)
        arguments.push_back(item.llvmCodegen(instance));

    if (!func->getReturnType()->isVoidTy())
        return instance.Builder.CreateCall(func, arguments, ident->toString());

    instance.Builder.CreateCall(func, arguments);
    return nullptr;
}


llvm::Value* Var::llvmCodegen(LLVMBackend& instance) {
    llvm::Type* type = var_type->llvmCodegen(instance);

    llvm::Value* ret;
    llvm::Value* init = nullptr;

    if (initialized)
        init = value.llvmCodegen(instance);

    if (!instance.IsLocalScope) {
        // TODO
        auto* var = new llvm::GlobalVariable(
                *instance.LModule, type, is_const, llvm::GlobalVariable::InternalLinkage,
                nullptr, var_ident->toString()
                );

        if (init) {
            const auto val = llvm::dyn_cast<llvm::Constant>(init);
            var->setInitializer(val);
        } ret = var;
        instance.SymManager.lookupDecl(this->var_ident).ptr = var;
    } else {
        llvm::AllocaInst* var_alloca = instance.Builder.CreateAlloca(type, nullptr, var_ident->toString());
        if (init != nullptr) {
            instance.Builder.CreateStore(init, var_alloca, is_volatile);
        }

        ret = var_alloca;
        instance.SymManager.lookupDecl(this->var_ident).ptr = var_alloca;
    }

    return ret;
}

void GenerateObjectFileLLVM(const LLVMBackend& instance) {
    const auto target_triple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string err;
    if (const auto target = llvm::TargetRegistry::lookupTarget(target_triple, err)) {
        const auto cpu  = "generic";
        const auto feat = "";

        const llvm::TargetOptions opt;
        const auto machine = target->createTargetMachine(target_triple, cpu, feat, opt, llvm::Reloc::PIC_);

        instance.LModule->setDataLayout(machine->createDataLayout());
        instance.LModule->setTargetTriple(target_triple);

        std::error_code f_ec;
        llvm::raw_fd_ostream dest(SW_OUTPUT, f_ec, llvm::sys::fs::OF_None);

        if (f_ec) {
            llvm::errs() << "Could not open output file! " << f_ec.message();
            return;
        }

        llvm::legacy::PassManager pass{};

        if ( constexpr auto file_type = llvm::CodeGenFileType::ObjectFile;
             machine->addPassesToEmitFile(pass, dest, nullptr, file_type)
            ) { llvm::errs() << "TargetMachine can't emit a file of this type";
            return;
        }

        pass.run(*instance.LModule);
        dest.flush();
    } else {
        llvm::errs() << err;
    }
}

void printIR(const LLVMBackend& instance) {
    llvm::verifyModule(*instance.LModule, &llvm::errs());
    instance.LModule->print(llvm::outs(), nullptr);
}
