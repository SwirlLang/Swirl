#include "types/SwTypes.h"
#include <cassert>
#include <string>
#include <ranges>
#include <unordered_map>

#include <backend/LLVMBackend.h>
#include <managers/ModuleManager.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <parser/Parser.h>


// ReSharper disable all CppUseStructuredBinding


class NewScope {
    bool prev_ls_cache{};
    LLVMBackend& instance;

public:
     explicit NewScope(LLVMBackend& inst): instance(inst) {
         prev_ls_cache = inst.IsLocalScope;
         instance.IsLocalScope = true;
     }

    ~NewScope() {
         instance.IsLocalScope = prev_ls_cache;
         instance.ChildHasReturned = false;
     }
};



void codegenChildrenUntilRet(LLVMBackend& instance, std::vector<std::unique_ptr<Node>>& children) {
    for (const auto& child : children) {
        if (child->getNodeType() == ND_RET) {
            child->llvmCodegen(instance);
            instance.ChildHasReturned = true;
            return;
        } if (child->getNodeType() != ND_INVALID) child->llvmCodegen(instance);
    }
}

llvm::Value* IntLit::llvmCodegen(LLVMBackend& instance) {
    llvm::Value* ret = nullptr;

    if (instance.getBoundTypeState()->isIntegral()) {
        auto int_type = llvm::dyn_cast<llvm::IntegerType>(instance.getBoundLLVMType());
        assert(int_type);

        if (value.starts_with("0x"))
            ret = llvm::ConstantInt::get(int_type, value.substr(2), 16);
        else if (value.starts_with("0o"))
            ret = llvm::ConstantInt::get(int_type, value.substr(2), 8);
        else ret = llvm::ConstantInt::get(int_type, value, 10); 
    }

    else if (instance.getBoundTypeState()->isFloatingPoint()) {
        ret = llvm::ConstantFP::get(instance.getBoundLLVMType(), value);
    } else {
        throw std::runtime_error("Fatal: IntLit::llvmCodegen called but instance is neither in "
                                "integral nor FP state.");
    }
    return ret;
}

llvm::Value* FloatLit::llvmCodegen(LLVMBackend& instance) {
    return llvm::ConstantFP::get(instance.getBoundLLVMType(), value);
}

llvm::Value* StrLit::llvmCodegen(LLVMBackend& instance) {
    auto struct_instance = instance.Builder.CreateAlloca(instance.getBoundLLVMType());
    auto literal_mem = instance.Builder.CreateStructGEP(instance.getBoundLLVMType(), struct_instance, 0);

    instance.Builder.CreateStore(
        llvm::ConstantDataArray::getString(instance.Context, value, false),
        literal_mem
        );

    return instance.Builder.CreateLoad(instance.getBoundLLVMType(), struct_instance);
}

/// Writes the array literal to 'BoundMemory' if not null, otherwise creates a temporary and
/// returns a load of it.
llvm::Value* ArrayNode::llvmCodegen(LLVMBackend& instance) {
    // the array-type which is enclosed within a struct
    assert(instance.getBoundLLVMType()->isStructTy());
    llvm::Type*  arr_type = instance.getBoundLLVMType()->getStructElementType(0);
    llvm::Value* ptr  = nullptr;  // the to-be-calculated pointer to where the array is supposed to be written
    llvm::Value* bound_mem_cache = nullptr;

    // if this flag is set, the literal shall be written to the storage that it represents
    if (instance.BoundMemory) {
        // set llvm_value = array member
        auto base_ptr = instance.Builder.CreateStructGEP(instance.getBoundLLVMType(), instance.BoundMemory, 0);

        for (auto [i, element] : std::views::enumerate(elements)) {
            ptr = instance.Builder.CreateGEP(arr_type, base_ptr, {instance.toLLVMInt(0), instance.toLLVMInt(i)});
            if (element.expr_type->getSwType() == Type::ARRAY) {
                bound_mem_cache = instance.BoundMemory;
                instance.BoundMemory = ptr;
                element.llvmCodegen(instance);
                continue;
            } instance.Builder.CreateStore(element.llvmCodegen(instance), ptr);
        }
        if (bound_mem_cache) instance.BoundMemory = bound_mem_cache;
        return nullptr;
    }

    auto tmp = instance.Builder.CreateAlloca(instance.getBoundLLVMType());
    auto base_ptr = instance.Builder.CreateStructGEP(instance.getBoundLLVMType(), tmp, 0);

    for (auto [i, element] : std::views::enumerate(elements)) {
        ptr = instance.Builder.CreateGEP(arr_type, base_ptr, {instance.toLLVMInt(0), instance.toLLVMInt(i)});
        if (element.expr_type->getSwType() == Type::ARRAY) {
            bound_mem_cache = instance.BoundMemory;
            instance.BoundMemory = ptr;
            element.llvmCodegen(instance);
            continue;
        } instance.Builder.CreateStore(element.llvmCodegen(instance), ptr);
    }

    if (bound_mem_cache) instance.BoundMemory = bound_mem_cache;
    return instance.Builder.CreateLoad(instance.getBoundLLVMType(), tmp);
}


llvm::Value* Ident::llvmCodegen(LLVMBackend& instance) {
    const auto e = instance.SymMan.lookupDecl(this->value);
    // if (e.is_param) { return e.llvm_value; }
    if (instance.IsAssignmentLHS) { return e.llvm_value; }

    return e.is_param
    ? instance.castIfNecessary(e.swirl_type, e.llvm_value)
    : instance.castIfNecessary(
        e.swirl_type, instance.Builder.CreateLoad(
            e.swirl_type->llvmCodegen(instance), e.llvm_value));
}

llvm::Value* ImportNode::llvmCodegen(LLVMBackend &instance) {
    return nullptr;  // TODO
}


llvm::Value* Function::llvmCodegen(LLVMBackend& instance) {
    std::recursive_mutex mtx;
    std::lock_guard guard(mtx);

    const auto fn_sw_type = dynamic_cast<FunctionType*>(instance.SymMan.lookupType(ident));

    auto linkage = this->is_exported ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage;

    auto*               fn_type  = llvm::dyn_cast<llvm::FunctionType>(fn_sw_type->llvmCodegen(instance));
    llvm::Function*     func     = llvm::Function::Create(fn_type, linkage, ident->toString(), instance.LModule.get());

    llvm::BasicBlock*   entry_bb = llvm::BasicBlock::Create(instance.Context, "entry", func);

    NewScope _(instance);
    instance.Builder.SetInsertPoint(entry_bb);

    for (int i = 0; i < params.size(); i++) {
        const auto p_name = params[i].var_ident;
        const auto param = func->getArg(i);
        // param->setName(p_name->toString());

        instance.SymMan.lookupDecl(p_name).llvm_value = func->getArg(i);
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
        llvm::Value* ret = value.llvmCodegen(instance);
        instance.Builder.CreateRet(ret);
        return nullptr;
    }

    instance.Builder.CreateRetVoid();
    return nullptr;
}


llvm::Value* Op::llvmCodegen(LLVMBackend& instance) {
    using SwNode = std::unique_ptr<Node>;
    using NodesVec = std::vector<SwNode>;

    switch (op_type) {
        case UNARY_ADD:
            return operands.back()->llvmCodegen(instance);

        case UNARY_SUB:
            return instance.Builder.CreateNeg(operands.back()->llvmCodegen(instance));

        case BINARY_ADD: {
            auto lhs = operands.at(0)->llvmCodegen(instance);
            auto rhs = operands.at(1)->llvmCodegen(instance);

            if (instance.getBoundTypeState()->isIntegral()) {
                return instance.Builder.CreateAdd(lhs, rhs);
            }

            assert(lhs->getType()->isFloatingPointTy() && rhs->getType()->isFloatingPointTy());
            return instance.Builder.CreateFAdd(lhs, rhs);
        }

        case BINARY_SUB: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);
            if (instance.getBoundTypeState()->isIntegral()) {
                return instance.Builder.CreateSub(lhs, rhs);
            }
            return instance.Builder.CreateFSub(lhs, rhs);
        }

        case MUL: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            if (instance.getBoundTypeState()->isIntegral()) {
                return instance.Builder.CreateMul(lhs, rhs);
            }

            return instance.Builder.CreateFMul(lhs, rhs);
        }

        case DEREFERENCE: {
            const auto entry = instance.SymMan.lookupDecl(operands.at(0)->getIdentInfo());
            if (entry.is_param)
                return entry.llvm_value;
            return instance.Builder.CreateLoad(entry.llvm_value->getType(), entry.llvm_value);
        }

        case ADDRESS_TAKING: {
            auto lookup = instance.SymMan.lookupDecl(operands.at(0)->getIdentInfo());
            return lookup.llvm_value;
        }

        case DIV: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            if (instance.getBoundTypeState()->isFloatingPoint()) {
                return instance.Builder.CreateFDiv(lhs, rhs);
            }
            if (!instance.getBoundTypeState()->isUnsigned()) {
                return instance.Builder.CreateSDiv(lhs, rhs);
            }

            return instance.Builder.CreateUDiv(lhs, rhs);
        }

        case CAST_OP:
            return operands.at(0)->llvmCodegen(instance);


        case LOGICAL_EQUAL: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            assert(lhs->getType() == rhs->getType());

            if (lhs->getType()->isFloatingPointTy()) {
                return instance.Builder.CreateFCmpOEQ(lhs, rhs);
            }

            if (lhs->getType()->isIntegerTy()) {
                return instance.Builder.CreateICmpEQ(lhs, rhs);
            }
        }

        case LOGICAL_NOTEQUAL: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            assert(lhs->getType() == rhs->getType());

            if (lhs->getType()->isFloatingPointTy()) {
                return instance.Builder.CreateFCmpONE(lhs, rhs);
            }

            if (lhs->getType()->isIntegerTy()) {
                return instance.Builder.CreateICmpNE(lhs, rhs);
            }
            throw;
        }

        case LOGICAL_AND: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            return instance.Builder.CreateLogicalAnd(lhs, rhs);
        }

        case LOGICAL_NOT: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            return instance.Builder.CreateNot(lhs);
        }

        case LOGICAL_OR: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            return instance.Builder.CreateLogicalOr(lhs, rhs);
        }

        case GREATER_THAN: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return instance.Builder.CreateFCmpOGT(lhs, rhs);
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return instance.Builder.CreateICmpUGT(lhs, rhs);
            }

            return instance.Builder.CreateICmpSGT(lhs, rhs);
        }

        case GREATER_THAN_OR_EQUAL: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return instance.Builder.CreateFCmpOGE(lhs, rhs);
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return instance.Builder.CreateICmpUGE(lhs, rhs);
            }

            return instance.Builder.CreateICmpSGE(lhs, rhs);
        }

        case LESS_THAN: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return instance.Builder.CreateFCmpOLT(lhs, rhs);
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return instance.Builder.CreateICmpULT(lhs, rhs);
            }

            return instance.Builder.CreateICmpSLT(lhs, rhs);
        }

        case LESS_THAN_OR_EQUAL: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return instance.Builder.CreateFCmpOLE(lhs, rhs);
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return instance.Builder.CreateICmpULE(lhs, rhs);
            }

            return instance.Builder.CreateICmpSLE(lhs, rhs);
        }

        case ASSIGNMENT: {
            instance.IsAssignmentLHS = true;
            auto lhs = operands.at(0)->llvmCodegen(instance);
            instance.IsAssignmentLHS = false;
            instance.setBoundTypeState(instance.fetchSwType(operands.at(0)));
            instance.Builder.CreateStore(operands.at(1)->llvmCodegen(instance), lhs);
            instance.restoreBoundTypeState();
            return nullptr;
        }

        case INDEXING_OP: {
            auto operand_sw_ty = dynamic_cast<ArrayType*>(instance.fetchSwType(operands.at(0)));
            auto operand_ty = operand_sw_ty->llvmCodegen(instance);

            auto ass_state_cache = instance.IsAssignmentLHS;
            instance.IsAssignmentLHS = true;
            auto arr_ptr = instance.Builder.CreateStructGEP(operand_ty, operands.at(0)->llvmCodegen(instance), 0);
            instance.IsAssignmentLHS = ass_state_cache;

            llvm::Value* second_op;
            if (instance.IsAssignmentLHS) {
                instance.IsAssignmentLHS = false;
                second_op = operands.at(1)->llvmCodegen(instance);
                instance.IsAssignmentLHS = true;
            } else second_op = operands.at(1)->llvmCodegen(instance);

            auto element_ptr = instance.Builder.CreateGEP(
                operand_ty->getStructElementType(0),
                arr_ptr,
                {instance.toLLVMInt(0), second_op}
                );

            if (instance.IsAssignmentLHS) {
                return element_ptr;
            } return instance.Builder.CreateLoad(operand_sw_ty->of_type->llvmCodegen(instance), element_ptr);
        }

        case DOT:
            throw;
        default: break;
    }

    using namespace std::string_view_literals;

    if (value.ends_with("=") && ("*-+/~&^"sv.find(value.at(0)) != std::string::npos)) {
        auto op = std::make_unique<Op>(std::string_view{value.data(), 1}, 2);
        op->operands.push_back(std::move(operands.at(0)));
        op->operands.push_back(std::move(operands.at(1)));

        instance.setBoundTypeState(instance.fetchSwType(op->operands.at(1)));
        auto rhs = op->llvmCodegen(instance);
        instance.restoreBoundTypeState();

        instance.IsAssignmentLHS = true;
        auto lhs = op->operands.at(0)->llvmCodegen(instance);
        instance.IsAssignmentLHS = false;

        instance.Builder.CreateStore(rhs, lhs);
        return nullptr;
    }

    throw;
}


llvm::Value* LLVMBackend::castIfNecessary(Type* source_type, llvm::Value* subject) {
    if (getBoundTypeState() != source_type) {
        if (getBoundTypeState()->isIntegral()) {
            if (getBoundTypeState()->isUnsigned()) {
                return Builder.CreateZExtOrTrunc(subject, getBoundLLVMType());
            } return Builder.CreateSExtOrTrunc(subject, getBoundLLVMType());
        }

        if (getBoundTypeState()->isFloatingPoint()) {
            if (source_type->isFloatingPoint()) {
                return Builder.CreateFPCast(subject, getBoundLLVMType());
            }

            if (source_type->isIntegral()) {
                if (source_type->isUnsigned()) {
                    return Builder.CreateUIToFP(subject, getBoundLLVMType());
                } else {
                    return Builder.CreateSIToFP(subject, getBoundLLVMType());
                }
            }
        }
    } return subject;
}


llvm::Value* Expression::llvmCodegen(LLVMBackend& instance) {
    assert(expr_type != nullptr);
    assert(expr.size() == 1);

    instance.setBoundTypeState(expr_type);
    const auto val = expr.back()->llvmCodegen(instance);
    instance.restoreBoundTypeState();

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

    IdentInfo* fn_name = ident.getIdentInfo();

    llvm::Function* func = instance.LModule->getFunction(fn_name->toString());
    if (!func) {
        auto fn = llvm::Function::Create(
            llvm::dyn_cast<llvm::FunctionType>(instance.SymMan.lookupType(ident.getIdentInfo())->llvmCodegen(instance)),
            llvm::GlobalValue::ExternalLinkage,
            ident.getIdentInfo()->toString(),
            instance.LModule.get()
            );
        func = instance.LModule->getFunction(fn_name->toString());
    }

    std::vector<llvm::Value*> arguments{};
    arguments.reserve(args.size());

    instance.InArgumentContext = true;
    for (auto& item : args)
        arguments.push_back(item.llvmCodegen(instance));
    instance.InArgumentContext = false;

    if (!func->getReturnType()->isVoidTy()) {
        Type* ret_type = nullptr;

        auto fn_type = dynamic_cast<FunctionType*>(instance.SymMan.lookupType(ident.value));
        ret_type = fn_type->ret_type;

        return instance.castIfNecessary(
            ret_type,
            instance.Builder.CreateCall(func, arguments, ident.value->toString())
            );
    }

    instance.Builder.CreateCall(func, arguments);
    return nullptr;
}


llvm::Value* Var::llvmCodegen(LLVMBackend& instance) {
    llvm::Type* type = var_type->llvmCodegen(instance);

    llvm::Value* ret;
    llvm::Value* init = nullptr;

    auto linkage = this->is_exported ? llvm::GlobalVariable::ExternalLinkage : llvm::GlobalVariable::InternalLinkage;

    
    if (!instance.IsLocalScope) {
        // TODO
        auto* var = new llvm::GlobalVariable(
                *instance.LModule, type, is_const, linkage,
                nullptr, var_ident->toString()
                );

        if (initialized) {
            instance.BoundMemory = var;
            init = value.llvmCodegen(instance);
            const auto val = llvm::dyn_cast<llvm::Constant>(init);
            var->setInitializer(val);
            instance.BoundMemory = nullptr;
        } ret = var;
        instance.SymMan.lookupDecl(this->var_ident).llvm_value = var;
    } else {
        llvm::AllocaInst* var_alloca = instance.Builder.CreateAlloca(type, nullptr, var_ident->toString());

        if (initialized) {
            instance.BoundMemory = var_alloca;
            init = value.llvmCodegen(instance);
            if (init != nullptr)
                instance.Builder.CreateStore(init, var_alloca, is_volatile);
            instance.BoundMemory = nullptr;
        }
        
        ret = var_alloca;
        instance.SymMan.lookupDecl(this->var_ident).llvm_value = var_alloca;
    }

    return ret;
}

void LLVMBackend::codegenTheFunction(IdentInfo* id) {
    auto ip_cache = this->Builder.saveIP();
    if (!GlobalNodeJmpTable.contains(id)) {
        ModuleMap.get(id->getModulePath()).NodeJmpTable.at(id)->llvmCodegen(*this);
        this->Builder.restoreIP(ip_cache);
        return;
    } GlobalNodeJmpTable[id]->llvmCodegen(*this);
    this->Builder.restoreIP(ip_cache);
}


void GenerateObjectFileLLVM(const LLVMBackend& instance) {
    // const auto target_triple = llvm::sys::getDefaultTargetTriple();
    // llvm::InitializeAllTargetInfos();
    // llvm::InitializeAllTargets();
    // llvm::InitializeAllTargetMCs();
    // llvm::InitializeAllAsmParsers();
    // llvm::InitializeAllAsmPrinters();
    //
    // std::string err;
    // if (const auto target = llvm::TargetRegistry::lookupTarget(target_triple, err)) {
    //     const auto cpu  = "generic";
    //     const auto feat = "";
    //
    //     const llvm::TargetOptions opt;
    //     const auto machine = target->createTargetMachine(target_triple, cpu, feat, opt, llvm::Reloc::PIC_);
    //
    //     instance.LModule->setDataLayout(machine->createDataLayout());
    //     instance.LModule->setTargetTriple(target_triple);
    //
    //     std::error_code f_ec;
    //     llvm::raw_fd_ostream dest(OUTPUT_FILE_PATH, f_ec, llvm::sys::fs::OF_None);
    //
    //     if (f_ec) {
    //         llvm::errs() << "Could not open output file! " << f_ec.message();
    //         return;
    //     }
    //
    //     llvm::legacy::PassManager pass{};
    //
    //     if ( constexpr auto file_type = llvm::CodeGenFileType::ObjectFile;
    //          machine->addPassesToEmitFile(pass, dest, nullptr, file_type)
    //         ) { llvm::errs() << "TargetMachine can't emit a file of this type";
    //         return;
    //     }
    //
    //     pass.run(*instance.LModule);
    //     dest.flush();
    // } else {
    //     llvm::errs() << err;
    // }
}
