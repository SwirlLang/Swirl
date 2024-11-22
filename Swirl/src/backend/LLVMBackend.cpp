#include <iostream>
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
#include <llvm/IR/LegacyPassManagers.h>
#include <llvm/IR/Verifier.h>



extern std::string SW_OUTPUT;

// this struct must be instantiated BEFORE IRinstance.Builder::SetInsertPoint is called
struct NewScope {
    bool prev_ls_cache{};
    LLVMBackend& instance;

     explicit NewScope(LLVMBackend& inst): instance(inst) {
         prev_ls_cache = inst.IsLocalScope;
         inst.IsLocalScope = true;
         inst.SymManager.emplace_back();
     }

    ~NewScope() {
         instance.IsLocalScope = prev_ls_cache;
         instance.SymManager.pop_back();
         instance.ChildHasReturned = false;
     }
};


void codegenChildrenUntilRet(LLVMBackend& instance, std::vector<std::unique_ptr<Node>>& children) {
    for (const auto& child : children) {
        if (child->getType() == ND_RET) {
            child->llvmCodegen(instance);
            instance.ChildHasReturned = true;
            return;
        } child->llvmCodegen(instance);
    }
}


llvm::Value* llvmValueToFloat(llvm::Value* val, llvm::Type* dest, LLVMBackend& instance) {
    if (val->getType()->isSingleValueType())
        return instance.Builder.CreateSIToFP(val, dest);
    return instance.Builder.CreateUIToFP(val, dest);
}

llvm::Value* llvmValueToIntegral(llvm::Value* val, llvm::Type* dest, LLVMBackend& instance) {
    if (dest->isSingleValueType())
        return instance.Builder.CreateFPToSI(val, dest);
    return instance.Builder.CreateFPToUI(val, dest);
}

llvm::Value* IntLit::llvmCodegen(LLVMBackend& instance) {
    llvm::Value* ret;
    if (getValue().starts_with("0x"))
        ret = llvm::ConstantInt::get(instance.IntegralTypeState, getValue().substr(2), 16);  // edge-case of "0x" unhandled
    else if (getValue().starts_with("0o"))
        ret = llvm::ConstantInt::get(instance.IntegralTypeState, getValue().substr(2), 8);  // edge-case of "0o" unhandled
    else ret = llvm::ConstantInt::get(instance.IntegralTypeState, getValue(), 10);

    // TODO: handle octal and hexal radices
    if (instance.LatestBoundIsFloating) {
        ret = llvm::ConstantFP::get(instance.FloatTypeState, getValue());
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
        const auto e = instance.SymManager.lookupSymbol(this->value);
        if (e.is_param) return e.ptr;
        return instance.Builder.CreateLoad(e.type, e.ptr);
}

llvm::Value* Function::llvmCodegen(LLVMBackend& instance) {
    std::vector<llvm::Type*> param_types;

    param_types.reserve(params.size());
    for (const auto& param : params)
        param_types.push_back(instance.SymManager.lookupType(param.var_type).type);

    llvm::FunctionType* f_type = llvm::FunctionType::get(instance.SymManager.lookupType(this->ret_type).type, param_types, false);
    llvm::Function*     func   = llvm::Function::Create(f_type, llvm::GlobalValue::InternalLinkage, ident, instance.LModule.get());

    llvm::BasicBlock*   BB     = llvm::BasicBlock::Create(instance.Context, "", func);
    NewScope _(instance);
    instance.Builder.SetInsertPoint(BB);

    for (int i = 0; i < params.size(); i++) {
        const auto p_name = params[i].var_ident;
        const auto param = func->getArg(i);

        param->setName(p_name);
        instance.SymManager.back()[p_name] = {.ptr = param, .type = param_types[i], .is_param = true};
    }

    // for (const auto& child : this->children)
    //     child->codegen();

    codegenChildrenUntilRet(instance, children);
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
    static std::unordered_map<std::pair<std::string, uint8_t>, std::function<llvm::Value*(NodesVec&)>, PairHash>
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
            const auto entry = instance.SymManager.lookupSymbol(operands.at(0)->getValue());
            if (entry.is_param)
                return entry.ptr;
            return instance.Builder.CreateLoad(entry.ptr->getType(), entry.ptr);
        }},

        {{"&", 1}, [&instance](const NodesVec& operands) -> llvm::Value* {
            auto lookup = instance.SymManager.lookupSymbol(operands.at(0)->getValue());
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

        {{".", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
            auto entry = instance.SymManager.lookupSymbol(operands.at(0)->getValue());  // LHS symT entry
            auto aggregate_t = instance.SymManager.lookupType(entry.bound_type);


            // member index and type pair
            std::cout << "MEMBER NAME: " << operands.at(1)->getValue() << std::endl;
            auto [member_index, member_type] = aggregate_t.fields->at(operands.at(1)->getValue());
            auto field_ptr = instance.Builder.CreateGEP(
                entry.type, entry.ptr, {IntLit{"0"}.llvmCodegen(instance), IntLit{std::to_string(member_index)}.llvmCodegen(instance)});

            if (instance.IsAssignmentLHS)
                return field_ptr;
            return instance.Builder.CreateLoad(instance.SymManager.lookupType(member_type).type, field_ptr);
        }}

        // {{">", 2}, [&instance](const NodesVec& operands) -> llvm::Value* {
        // }}
    };

    return OpTable[{this->value, arity}](operands);
}


llvm::Value* Expression::llvmCodegen(LLVMBackend& instance) {
    std::stack<llvm::Value*> eval{};

    llvm::Type* e_type;
    if (!expr_type.empty()) {
        e_type = instance.SymManager.lookupType(expr_type).type;

        if (e_type->isIntegerTy())
            instance.setIntegralTypeState(e_type);
        else if (e_type->isFloatingPointTy())
            instance.setFloatTypeState(e_type);
    }

    const auto val = expr.back()->llvmCodegen(instance);

    if (!expr_type.empty()) {
        if (e_type->isIntegerTy())
            instance.restoreIntegralTypeState();
        else if (e_type->isFloatingPointTy())
            instance.restoreFloatTypeState();
    }

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
    const auto parent = instance.Builder.GetInsertBlock()->getParent();
    const auto if_block = llvm::BasicBlock::Create(instance.Context, "if", parent);
    const auto else_block = llvm::BasicBlock::Create(instance.Context, "else", parent);
    const auto merge_block = llvm::BasicBlock::Create(instance.Context, "merge", parent);

    const auto if_cond = this->bool_expr.llvmCodegen(instance);
    if (!if_cond)
        throw std::runtime_error("condition is null");

    std::vector false_blocks = {else_block};
    instance.Builder.CreateCondBr(if_cond, if_block, else_block);
    {
        NewScope _(instance);
        instance.Builder.SetInsertPoint(if_block);

        // for (auto& child : if_children)
        //     child->codegen();
        codegenChildrenUntilRet(instance, if_children);

        if (!instance.ChildHasReturned)
            instance.Builder.CreateBr(merge_block);
    }

    // elif(s)
    for (std::size_t i = 1; auto& [cond, children] : elif_children) {
        NewScope _(instance);
        if (i == 1) {
            instance.Builder.SetInsertPoint(if_block);
            else_block->setName("elif");
            false_blocks.push_back(llvm::BasicBlock::Create(instance.Context, "elif", parent));
            instance.Builder.CreateCondBr(cond.llvmCodegen(instance), else_block, false_blocks.back());
            instance.Builder.SetInsertPoint(else_block);
        } else {
            instance.Builder.SetInsertPoint(false_blocks.at(false_blocks.size() - 2));
            false_blocks.push_back(llvm::BasicBlock::Create(instance.Context, "elif", parent));
            instance.Builder.CreateCondBr(cond.llvmCodegen(instance), false_blocks.at(false_blocks.size() - 2), false_blocks.back());
            instance.Builder.SetInsertPoint(false_blocks.at(false_blocks.size() - 2));
        }

        // for (auto& child : children)
        //     child->codegen();
        codegenChildrenUntilRet(instance, children);

        if (i == elif_children.size() && !instance.ChildHasReturned)
            instance.Builder.CreateBr(merge_block);
        i++;
    }

    // else
    {
        NewScope _(instance);
        instance.Builder.SetInsertPoint(false_blocks.back());
        false_blocks.back()->setName("else");

        // for (const auto& child : else_children)
        //     child->codegen();
        codegenChildrenUntilRet(instance, else_children);

        if (!instance.ChildHasReturned)
            instance.Builder.CreateBr(merge_block);
    }

    instance.Builder.SetInsertPoint(merge_block);
    return nullptr;
}

llvm::Value* Struct::llvmCodegen(LLVMBackend& instance) {
    std::vector<llvm::Type*> types;
    std::vector<std::string> names;
    std::vector<std::string> type_names;

    types.reserve(members.size());
    names.reserve(members.size());
    type_names.reserve(members.size());

    for (const auto& mem : members)
        if (mem->getType() == ND_VAR) {
            const auto field = dynamic_cast<Var*>(mem.get());
            types.push_back(instance.SymManager.lookupType(field->var_type).type);
            names.push_back(field->var_ident);
            type_names.push_back(field->var_type);
        }

    auto* struct_ = llvm::StructType::get(instance.Context);
    struct_->setName(ident);
    struct_->setBody(types);

    if (!instance.IsLocalScope) {
        TableEntry entry;
        entry.type = struct_;
        entry.is_type = true;

        entry.fields = std::unordered_map<std::string, std::pair<std::size_t, std::string>>{};
        for (std::size_t i = 0; const auto& item : type_names) {
            entry.fields.value()[names[i]] = std::pair{i, item};
            i++;
        }

        instance.SymManager.registerGlobalType(ident,  std::move(entry));
    }

    else {
        TableEntry entry{};
        entry.type = struct_;
        entry.is_type = true;

        entry.fields = std::unordered_map<std::string, std::pair<std::size_t, std::string>>{};
        for (std::size_t i = 0; const auto& item : type_names) {
            entry.fields.value()[names[i]] = std::pair{i, item};
            i++;
        }
        instance.SymManager.back()[ident] = entry;
    }
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

    const auto expr  = condition.llvmCodegen(instance);

    instance.Builder.SetInsertPoint(cond_block);
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

    llvm::Function* func = instance.LModule->getFunction(ident);
    if (!func)
        throw std::runtime_error("No such function defined");

    std::vector<llvm::Value*> arguments{};
    arguments.reserve(args.size());

    for (auto& item : args)
        arguments.push_back(item.llvmCodegen(instance));

    if (!func->getReturnType()->isVoidTy())
        return instance.Builder.CreateCall(func, arguments, ident);

    instance.Builder.CreateCall(func, arguments);
    return nullptr;
}


llvm::Value* Var::llvmCodegen(LLVMBackend& instance) {
    llvm::Type* type = instance.SymManager.lookupType(var_type).type;

    llvm::Value* ret;
    llvm::Value* init = nullptr;

    if (initialized)
        init = value.llvmCodegen(instance);

    if (!instance.IsLocalScope) {
        // TODO
        auto* var = new llvm::GlobalVariable(
                *instance.LModule, type, is_const, llvm::GlobalVariable::InternalLinkage,
                nullptr, var_ident
                );

        if (init) {
            const auto val = llvm::dyn_cast<llvm::Constant>(init);
            var->setInitializer(val);
        } ret = var;
    } else {
        llvm::AllocaInst* var_alloca = instance.Builder.CreateAlloca(type, nullptr, var_ident);
        if (init != nullptr) {
            instance.Builder.CreateStore(init, var_alloca, is_volatile);
        }

        TableEntry entry{};
        entry.ptr = var_alloca;
        entry.type = instance.SymManager.lookupType(var_type).type;
        entry.is_volatile = is_volatile;
        entry.bound_type = var_type;

        instance.SymManager.back()[var_ident] = std::move(entry);
        ret = var_alloca;
    }

    return ret;
}

void GenerateObjectFileLLVM(LLVMBackend& instance) {
    const auto target_triple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string err{};
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

void Condition::print() {
    std::cout <<
        std::format("Condition: if-children-size: {}, else-children-size: {}", if_children.size(), elif_children.size())
    << std::endl;

    std::cout << "IF-children:" << std::endl;
    for (const auto& a : if_children) {
        std::cout << "\t";
        a->print();
    }

    std::cout << "ELSE-children:" << std::endl;
    // for (const auto& a : elif_children) {
    //     for (const auto& a : if_children) {
    //         std::cout << "\t";
    //         a->print();
    //     }
    // }
}

void printIR(LLVMBackend& instance) {
    instance.LModule->print(llvm::outs(), nullptr);
}

void Var::print() {
    std::cout << std::format("Var: {}", var_ident) << std::endl;
}

void Function::print() {
    std::cout << std::format("func: {}", ident) << std::endl;
    for (const auto& a : children) {
        std::cout << "\t";
        a->print();
    }
}
