#include <cassert>
#include <string>
#include <unordered_map>

#include "CompilerInst.h"
#include "types/SwTypes.h"
#include "backend/LLVMBackend.h"
#include "parser/Parser.h"

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
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>


#define PRE_SETUP() LLVMBackend::SetupHandler GET_UNIQUE_NAME(backend_helper_){instance, this};
#define SET_BOUND_TYPE_STATE(x) LLVMBackend::BoundTypeStateHelper GET_UNIQUE_NAME(bound_ty)(instance, x);

// ReSharper disable all CppUseStructuredBinding


LLVMBackend::LLVMBackend(Parser& parser)
    : ModuleManager(parser.m_ModuleMap),
      LModule{
        std::make_unique<llvm::Module>(
            ModuleManager.getModuleUID(parser.m_FilePath),
            Context)
    }
    , AST(std::move(parser.AST))
    , SymMan(parser.SymbolTable)
{
    m_LatestBoundType.emplace(nullptr);
    m_ManglingContexts.emplace();

    static std::once_flag once_flag;
    std::call_once(once_flag, []{
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmParser();
        llvm::InitializeNativeTargetAsmPrinter();
    });

    std::string error;
    const auto target = llvm::TargetRegistry::lookupTarget(CompilerInst::TargetTriple, error);

    if (!target) {
        throw std::runtime_error("Failed to lookup target! " + error);
    }

    llvm::TargetOptions options;
    auto reloc_model = std::optional<llvm::Reloc::Model>();

    static std::once_flag _;
    std::call_once(_, [&] {
        TargetMachine = target->createTargetMachine(
            CompilerInst::TargetTriple, "generic", "", options, reloc_model
        );
    });

    LModule->setDataLayout(TargetMachine->createDataLayout());
    LModule->setTargetTriple(llvm::Triple(CompilerInst::TargetTriple).getTriple());
}


/// Creates and returns a CGValue out of an lvalue
CGValue CGValue::lValue(llvm::Value* lvalue) {
    return {lvalue, nullptr};
}

/// Creates and returns a CGValue out of an rvalue
CGValue CGValue::rValue(llvm::Value* rvalue) {
    return {nullptr, rvalue};
}

/// Returns the held lvalue
llvm::Value* CGValue::getLValue() {
    if (!m_LValue) {
        assert(m_RValue);
        assert(m_RValue->getType()->isPointerTy());
        return m_RValue;
    } return m_LValue;
}

/// If there's no held rvalue, creates and returns a load of the held lvalue, returns the held
/// rvalue otherwise.
llvm::Value* CGValue::getRValue(LLVMBackend& instance) {
    if (!m_RValue) {
        assert(m_LValue);
        return instance.Builder.CreateLoad(instance.getBoundLLVMType(), m_LValue);
    } return m_RValue;
}


std::string LLVMBackend::mangleString(IdentInfo* id) {
    auto decl_lookup = SymMan.lookupDecl(id);

    if (decl_lookup.node_ptr) {
        // do not mangle `extern "C"` symbols
        if (decl_lookup.node_ptr->isGlobal()) {
            if (dynamic_cast<GlobalNode*>(
                decl_lookup.node_ptr
                )->extern_attributes.contains("C")) {
                return id->toString();
            }
        }
    }

    if (decl_lookup.method_of) {
        auto type = decl_lookup.method_of;
        assert(type != nullptr);

        // unwrap the type if needed
        if (type->getTypeTag() == Type::REFERENCE)
            type = dynamic_cast<ReferenceType*>(type)->of_type;
        assert(type->getTypeTag() != Type::POINTER);

        return "__Sw_" + type->toString() +
            '_' + ModuleManager.getModuleUID(id->getModulePath()) + "_" + id->toString();
    } return "__Sw_" + ModuleManager.getModuleUID(id->getModulePath()) + '_' + id->toString();
}


void LLVMBackend::codegenChildrenUntilRet(NodesVec& children, llvm::Value* condition) {
    if (condition) {
        if (llvm::isa<llvm::ConstantInt>(condition)) {
            auto v = llvm::cast<llvm::ConstantInt>(condition);
            if (v->isZero()) {
                return;
            }
        }
    }

    for (const auto& child : children) {
        switch (child->getNodeType()) {
            case ND_BREAK:
            case ND_CONTINUE:
            case ND_RET:
                child->llvmCodegen(*this);
                return;
            case ND_INVALID:
                continue;
            default:
                child->llvmCodegen(*this);
        }
    }
}


CGValue IntLit::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    llvm::Value* ret = nullptr;

    if (instance.getBoundTypeState()->isIntegral()) {
        auto int_type = llvm::dyn_cast<llvm::IntegerType>(instance.getBoundLLVMType());
        assert(int_type);

        if (value.starts_with("0x"))
            ret = llvm::ConstantInt::get(int_type, value.substr(2), 16);
        else if (value.starts_with("0o"))
            ret = llvm::ConstantInt::get(int_type, value.substr(2), 8);
        else if (value.starts_with("0b"))
            ret = llvm::ConstantInt::get(int_type, value.substr(2), 2);
        else ret = llvm::ConstantInt::get(int_type, value, 10);
    }

    else if (instance.getBoundTypeState()->isFloatingPoint()) {
        ret = llvm::ConstantFP::get(instance.getBoundLLVMType(), value);
    } else if (instance.getBoundTypeState()->getTypeTag() == Type::BOOL) {
        ret = llvm::ConstantInt::get(llvm::Type::getInt32Ty(instance.Context), std::to_string(toInteger(value)), 10);
    }

    else {
        throw std::runtime_error(std::format("Fatal: IntLit::llvmCodegen called but instance is neither in "
                                "integral nor FP state. State: `{}`", instance.getBoundTypeState()->toString()));
    }
    return CGValue::rValue(ret);
}

CGValue FloatLit::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    return CGValue::rValue(llvm::ConstantFP::get(instance.getBoundLLVMType(), value));
}

CGValue StrLit::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    auto ptr = instance.Builder.CreateGlobalString(
        value, "", 0, instance.getLLVMModule(), false);

    return CGValue::rValue(llvm::ConstantStruct::get(
        llvm::dyn_cast<llvm::StructType>(GlobalTypeStr.llvmCodegen(instance)), {
            ptr,
            llvm::dyn_cast<llvm::Constant>(instance.toLLVMInt(value.size()))
        }));
}


CGValue BoolLit::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    return CGValue::rValue(llvm::ConstantInt::getBool(instance.Context, value));
}


/// Writes the array literal to 'BoundMemory' if not null, otherwise creates a temporary and
/// returns a load of it.
CGValue ArrayLit::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    assert(!elements.empty());
    Type* element_type = elements.at(0).expr_type;
    Type* sw_arr_type  = instance.SymMan.getArrayType(element_type, elements.size());

    if (!instance.isLocalScope()) {
        if (instance.BoundMemory) {
            auto arr_type = llvm::ArrayType::get(element_type->llvmCodegen(instance), elements.size());

            std::vector<llvm::Constant*> val_array;
            val_array.reserve(elements.size());
            for (auto& elem : elements) {
                instance.setBoundTypeState(element_type);
                val_array.push_back(llvm::dyn_cast<llvm::Constant>(
                    elem.llvmCodegen(instance).getRValue(instance)));
                instance.restoreBoundTypeState();
            }

            auto array_init = llvm::ConstantArray::get(arr_type, val_array);

            std::vector<llvm::Type*> members = {arr_type};

            auto const_struct = llvm::ConstantStruct::get(
                llvm::dyn_cast<llvm::StructType>(sw_arr_type->llvmCodegen(instance)),
                {array_init}
                );
            return CGValue::rValue(const_struct);
        }
    }

    // the array-type which is enclosed within a struct
    assert(instance.getBoundLLVMType()->isStructTy());
    llvm::Type*  arr_type = instance.getBoundLLVMType()->getStructElementType(0);
    llvm::Value* ptr  = nullptr;  // the to-be-calculated pointer to where the array is supposed to be written
    llvm::Value* bound_mem_cache = nullptr;

    // if this flag is set, the literal shall be written to the storage that it represents
    if (instance.BoundMemory) {
        // set llvm_value = array member
        auto base_ptr = instance.Builder.CreateStructGEP(instance.getBoundLLVMType(), instance.BoundMemory, 0);

        for (auto [i, element] : llvm::enumerate(elements)) {
            ptr = instance.Builder.CreateGEP(arr_type, base_ptr, {instance.toLLVMInt(0), instance.toLLVMInt(i)});
            if (element.expr_type->getTypeTag() == Type::ARRAY) {
                bound_mem_cache = instance.BoundMemory;
                instance.BoundMemory = ptr;
                instance.setBoundTypeState(element_type);
                element.llvmCodegen(instance);
                instance.restoreBoundTypeState();
                continue;
            } instance.Builder.CreateStore(element.llvmCodegen(instance).getRValue(instance), ptr);
        }
        if (bound_mem_cache) instance.BoundMemory = bound_mem_cache;
        return {nullptr, nullptr};
    }

    auto tmp = instance.Builder.CreateAlloca(instance.getBoundLLVMType());
    auto base_ptr = instance.Builder.CreateStructGEP(instance.getBoundLLVMType(), tmp, 0);


    for (auto [i, element] : llvm::enumerate(elements)) {
        ptr = instance.Builder.CreateGEP(arr_type, base_ptr, {instance.toLLVMInt(0), instance.toLLVMInt(i)});
        if (element.expr_type->getTypeTag() == Type::ARRAY) {
            bound_mem_cache = instance.BoundMemory;
            instance.BoundMemory = ptr;
            instance.setBoundTypeState(element_type);
            element.llvmCodegen(instance);
            instance.restoreBoundTypeState();
            continue;
        } instance.Builder.CreateStore(element.llvmCodegen(instance).getRValue(instance), ptr);
    }

    if (bound_mem_cache)
        instance.BoundMemory = bound_mem_cache;

    auto tmp_load = instance.Builder.CreateLoad(instance.getBoundLLVMType(), tmp);
    assert(tmp_load);
    return CGValue::rValue(tmp_load);
}


CGValue Ident::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    const auto e = instance.SymMan.lookupDecl(this->value);

    if (!instance.isLocalScope()) {
        auto global_var = llvm::dyn_cast<llvm::GlobalVariable>(e.llvm_value);
        return CGValue::rValue(global_var->getInitializer());
    }

    // parameters are passed by value
    if (e.is_param) {
        return CGValue::rValue(e.llvm_value);
    }

    // return e.is_param
    // ? CGValue::lValue(instance.castIfNecessary(e.swirl_type, e.llvm_value))
    // : CGValue::lValue();

    return {e.llvm_value, instance.castIfNecessary(
        e.swirl_type, instance.Builder.CreateLoad(
            e.swirl_type->llvmCodegen(instance), e.llvm_value))};
}


CGValue ImportNode::llvmCodegen([[maybe_unused]] LLVMBackend &instance) {
    return {nullptr, nullptr};
}


CGValue Scope::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    assert(instance.getCurrentParent() != nullptr);

    // create a new basic block if:
    // - no active basic blocks exist, or
    // - the last active basic block's last instruction is a terminator
    if (
        !instance.Builder.GetInsertBlock() ||
        instance.Builder.GetInsertBlock()->back().isTerminator()
        ) {

        auto bb = llvm::BasicBlock::Create(
           instance.Context,
           "scope_0",
           instance.getCurrentParent()
           );

        instance.Builder.SetInsertPoint(bb);
    }

    for (auto& node : children) {
        if (node->getNodeType() == ND_INVALID) {
            continue;
        } node->llvmCodegen(instance);
    } return {nullptr, nullptr};
}


CGValue Function::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    if (!generic_params.empty())
        return {};

    const auto fn_sw_type = dynamic_cast<FunctionType*>(instance.SymMan.lookupType(ident));

    auto name = is_extern || ident->toString() == "main" ?
        ident->toString() :
        instance.mangleString(ident);

    auto linkage = (
        is_exported || is_extern || ident->toString() == "main"
        ) ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage;

    auto*               fn_type  = llvm::dyn_cast<llvm::FunctionType>(fn_sw_type->llvmCodegen(instance));
    llvm::Function*     func     = llvm::Function::Create(fn_type, linkage, name, instance.LModule.get());

    instance.setCurrentParent(func);

    if (is_extern) {
        if (extern_attributes == "C")
            func->setCallingConv(llvm::CallingConv::C);
        return {nullptr, nullptr};
    }

    llvm::BasicBlock*   entry_bb = llvm::BasicBlock::Create(instance.Context, "entry", func);

    instance.Builder.SetInsertPoint(entry_bb);

    for (unsigned int i = 0; i < params.size(); i++) {
        const auto p_name = params[i].var_ident;
        [[maybe_unused]] const auto param = func->getArg(i);
        // param->setName(p_name->toString());

        instance.SymMan.lookupDecl(p_name).llvm_value = func->getArg(i);
    }

    instance.codegenChildrenUntilRet(children);
    if (!instance.Builder.GetInsertBlock()->back().isTerminator()
        || instance.Builder.GetInsertBlock()->empty())
        instance.Builder.CreateRetVoid();

    instance.setCurrentParent(nullptr);
    return {nullptr, nullptr};
}


CGValue ReturnStatement::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    if (value.expr) {
        assert(parent_fn_type->ret_type != nullptr);
        SET_BOUND_TYPE_STATE(parent_fn_type->ret_type);
        llvm::Value* ret = value.llvmCodegen(instance).getRValue(instance);
        instance.Builder.CreateRet(ret);
        return {nullptr, nullptr};
    }

    instance.Builder.CreateRetVoid();
    return {nullptr, nullptr};
}


CGValue Op::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    assert(common_type);
    SET_BOUND_TYPE_STATE(common_type);

    switch (op_type) {
        case UNARY_ADD:
            return operands.back()->llvmCodegen(instance);

        case UNARY_SUB:
            return
            CGValue::rValue(instance.Builder.CreateNeg(operands.back()->llvmCodegen(instance).getRValue(instance)));

        case BINARY_ADD: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            if (instance.getBoundTypeState()->isIntegral()) {
                return CGValue::rValue(instance.Builder.CreateAdd(lhs, rhs));
            }

            assert(lhs->getType()->isFloatingPointTy() && rhs->getType()->isFloatingPointTy());
            return CGValue::rValue(instance.Builder.CreateFAdd(lhs, rhs));
        }

        case BINARY_SUB: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            if (instance.getBoundTypeState()->isIntegral()) {
                return CGValue::rValue(instance.Builder.CreateSub(lhs, rhs));
            }
            return CGValue::rValue(instance.Builder.CreateFSub(lhs, rhs));
        }

        case MUL: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            if (instance.getBoundTypeState()->isIntegral()) {
                return CGValue::rValue(instance.Builder.CreateMul(lhs, rhs));
            }

            return CGValue::rValue(instance.Builder.CreateFMul(lhs, rhs));
        }

        case DEREFERENCE: {
            // preserve the l-value of the operand
            auto operand = getLHS()->llvmCodegen(instance);
            return CGValue::lValue(operand.getRValue(instance));
        }

        case ADDRESS_TAKING: {
            auto operand = getLHS()->getWrappedNodeOrInstance();
            Type* type = instance.fetchSwType(operand);

            // handle slice-creation for array types
            if (type->getTypeTag() == Type::ARRAY && !common_type->isPointerType()) {
                auto slice_type = instance.SymMan.getSliceType(type->getWrappedType(), is_mutable);

                auto slice_llvm_ty = slice_type->llvmCodegen(instance);

                auto slice_instance = instance.Builder.CreateAlloca(slice_llvm_ty);
                auto ptr_field = instance.Builder.CreateStructGEP(
                    slice_llvm_ty,
                    slice_instance,
                    0
                );

                auto size_field = instance.Builder.CreateStructGEP(
                    slice_llvm_ty,
                    slice_instance,
                    1
                );

                auto arr_instance_ptr = operand->llvmCodegen(instance);

                // calculate the pointer to the array
                auto element_ptr = instance.Builder.CreateStructGEP(
                    type->llvmCodegen(instance),
                    arr_instance_ptr.getLValue(),
                    0
                    );

                instance.Builder.CreateStore(element_ptr, ptr_field);
                instance.Builder.CreateStore(
                    instance.toLLVMInt(type->getAggregateSize()),
                    size_field
                    );

                return CGValue::rValue(llvm::dyn_cast<llvm::Value>(slice_instance));
            }

            if (operand->getNodeType() == ND_IDENT) {
                auto& tab_entry = instance.SymMan.lookupDecl(operand->getIdentInfo());
                auto op_address = tab_entry.llvm_value;
                assert(op_address->getType()->isPointerTy());

                instance.RefMemory = op_address;

                return CGValue{op_address, op_address};

            } throw;
        }

        case DIV: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            if (instance.getBoundTypeState()->isFloatingPoint()) {
                return CGValue::rValue(instance.Builder.CreateFDiv(lhs, rhs));
            }
            if (instance.getBoundTypeState()->isUnsigned()) {
                return CGValue::rValue(instance.Builder.CreateUDiv(lhs, rhs));
            }

            return CGValue::rValue(instance.Builder.CreateSDiv(lhs, rhs));
        }

        case MOD: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            // SemanticAnalysis ascertains that integral types are used
            if (instance.getBoundTypeState()->isUnsigned()) {
                return CGValue::rValue(instance.Builder.CreateURem(lhs, rhs));
            }

            return CGValue::rValue(instance.Builder.CreateSRem(lhs, rhs));
        }

        case CAST_OP: {
            assert(operands.at(1)->getNodeType() == ND_TYPE);
            SET_BOUND_TYPE_STATE(operands.at(1)->getSwType());
            return CGValue::rValue(instance.castIfNecessary(instance.fetchSwType(
                operands.at(0)), getLHS()->llvmCodegen(instance).getRValue(instance)));
            assert(0);
        }

        case LOGICAL_EQUAL: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            assert(lhs->getType() == rhs->getType());

            if (lhs->getType()->isFloatingPointTy()) {
                return CGValue::rValue(instance.Builder.CreateFCmpOEQ(lhs, rhs));
            }

            if (lhs->getType()->isIntegerTy()) {
                return CGValue::rValue(instance.Builder.CreateICmpEQ(lhs, rhs));
            }
            throw;
        }

        case LOGICAL_NOTEQUAL: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            assert(lhs->getType() == rhs->getType());

            if (lhs->getType()->isFloatingPointTy()) {
                return CGValue::rValue(instance.Builder.CreateFCmpONE(lhs, rhs));
            }

            if (lhs->getType()->isIntegerTy()) {
                return CGValue::rValue(instance.Builder.CreateICmpNE(lhs, rhs));
            }
            throw;
        }

        case LOGICAL_AND: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            return CGValue::rValue(instance.Builder.CreateLogicalAnd(lhs, rhs));
        }

        case LOGICAL_NOT: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            return CGValue::rValue(instance.Builder.CreateNot(lhs));
        }

        case LOGICAL_OR: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            return CGValue::rValue(instance.Builder.CreateLogicalOr(lhs, rhs));
        }

        case GREATER_THAN: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(instance.Builder.CreateFCmpOGT(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(instance.Builder.CreateICmpUGT(lhs, rhs));
            }

            return CGValue::rValue(instance.Builder.CreateICmpSGT(lhs, rhs));
        }

        case GREATER_THAN_OR_EQUAL: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(instance.Builder.CreateFCmpOGE(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(instance.Builder.CreateICmpUGE(lhs, rhs));
            }

            return CGValue::rValue(instance.Builder.CreateICmpSGE(lhs, rhs));
        }

        case LESS_THAN: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(instance.Builder.CreateFCmpOLT(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(instance.Builder.CreateICmpULT(lhs, rhs));
            }

            return CGValue::rValue(instance.Builder.CreateICmpSLT(lhs, rhs));
        }

        case LESS_THAN_OR_EQUAL: {
            llvm::Value* lhs = getLHS()->llvmCodegen(instance).getRValue(instance);
            llvm::Value* rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            auto lhs_type = instance.fetchSwType(operands.at(0));
            auto rhs_type = instance.fetchSwType(operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(instance.Builder.CreateFCmpOLE(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(instance.Builder.CreateICmpULE(lhs, rhs));
            }

            return CGValue::rValue(instance.Builder.CreateICmpSLE(lhs, rhs));
        }

        case ASSIGNMENT: {
            auto lhs = getLHS()->llvmCodegen(instance).getLValue();
            SET_BOUND_TYPE_STATE(common_type);
            auto rhs = getRHS()->llvmCodegen(instance).getRValue(instance);

            assert(lhs->getType()->isPointerTy());
            instance.Builder.CreateStore(rhs, lhs);
            return {};
        }

        case INDEXING_OP: {
            auto underlying_type = instance.fetchSwType(operands.at(0))->getWrappedType();

            auto operand_llvm_ty = instance.fetchSwType(operands.at(0))->llvmCodegen(instance);

            auto arr_ptr = instance.Builder.CreateStructGEP(
                operand_llvm_ty,  getLHS()->llvmCodegen(instance).getLValue(), 0);

            llvm::Value* second_op;
            instance.setBoundTypeState(instance.fetchSwType(operands.at(1)));
            second_op = operands.at(1)->llvmCodegen(instance).getRValue(instance);
            instance.restoreBoundTypeState();

            auto element_ptr = instance.Builder.CreateGEP(
                operand_llvm_ty->getStructElementType(0),
                arr_ptr,
                {instance.toLLVMInt(0), second_op}
                );

            assert(element_ptr != nullptr);
            instance.ComputedPtr = element_ptr;
            return CGValue::lValue(element_ptr);
            // return instance.Builder.CreateLoad(underlying_type->llvmCodegen(instance), element_ptr);
        }

        case DOT: {
            // when nested
            if (operands.at(0)->getNodeType() == ND_OP) {
                auto inst_ptr = operands.at(0)->llvmCodegen(instance).getLValue();

                auto field_node = dynamic_cast<Ident*>(operands.at(1).get());
                auto struct_ty  = dynamic_cast<StructType*>(instance.StructFieldType);

                auto field_ptr = instance.Builder.CreateStructGEP(
                    instance.StructFieldType->llvmCodegen(instance),
                    inst_ptr,
                    static_cast<unsigned int>
                    (struct_ty->field_offsets.at(field_node->full_qualification.front().name))
                );

                auto field_ty = instance.SymMan.lookupDecl(field_node->value).swirl_type;

                instance.StructFieldType = field_ty;
                instance.StructFieldPtr = field_ptr;
                instance.ComputedPtr = field_ptr;

                return CGValue::lValue(field_ptr);
                // instance.Builder.CreateLoad(
                //     field_ty->llvmCodegen(instance),
                //     field_ptr
                // );
            // ---- * ---- * ---- * ---- * ---- //
            } else {  // otherwise...
                auto inst_ptr = operands.at(0)->llvmCodegen(instance).getLValue();

                // in the special case when dot is using with pure-rvalues
                // e.g., functions returning struct instances
                if (!inst_ptr->getType()->isPointerTy()) {
                    auto tmp = instance.Builder.CreateAlloca(inst_ptr->getType());
                    instance.Builder.CreateStore(inst_ptr, tmp);
                    inst_ptr = tmp;
                }

                // fetch the instance's struct-type
                auto struct_sw_ty = instance.fetchSwType(operands.at(0));
                auto struct_ty = dynamic_cast<StructType*>(struct_sw_ty->getWrappedTypeOrInstance());

                // handle the special case of methods, lower them into regular function calls
                if (operands.at(1)->getNodeType() == ND_CALL) {
                    instance.setManglingContext({.encapsulator = struct_sw_ty});
                    instance.ComputedPtr = inst_ptr;  // set ComputedPtr for the FuncCall node to grab it
                    auto ret = operands.at(1)->llvmCodegen(instance);
                    instance.restoreManglingContext();
                    return ret;
                }

                assert(struct_ty != nullptr);
                auto field_node = dynamic_cast<Ident*>(operands.at(1).get());
                auto field_ptr = instance.Builder.CreateStructGEP(
                    struct_ty->llvmCodegen(instance),
                    inst_ptr,
                    static_cast<unsigned int>
                    (struct_ty->field_offsets.at(field_node->full_qualification.front().name))
                );

                // in the case of this operator, the common_type is supposed to be the type of the field
                // beind accessed, not an actual common type between operands
                auto field_type = common_type;

                instance.StructFieldPtr = field_ptr;
                instance.StructFieldType = field_type;
                instance.ComputedPtr = field_ptr;

                return CGValue::lValue(field_ptr);
                // instance.Builder.CreateLoad(
                //     field_type->llvmCodegen(instance),
                //     field_ptr
                // );
            }
        }
        default: break;
    }

    using namespace std::string_view_literals;

    if (value.ends_with("=") && ("+-*/%~&^"sv.find(value.at(0)) != std::string::npos)) {
        auto op = std::make_unique<Op>(std::string_view{value.data(), 1}, 2);
        op->operands.push_back(std::move(operands.at(0)));
        op->operands.push_back(std::move(operands.at(1)));
        op->common_type = common_type;

        instance.setBoundTypeState(instance.fetchSwType(op->operands.at(1)));
        auto rhs = op->llvmCodegen(instance).getRValue(instance);
        instance.restoreBoundTypeState();

        auto lhs = op->operands.at(0)->llvmCodegen(instance).getLValue();

        instance.Builder.CreateStore(rhs, lhs);
        return {};
    }

    throw;
}


CGValue TypeWrapper::llvmCodegen(LLVMBackend &instance) {
    return CGValue::rValue(llvm::UndefValue::get(type->llvmCodegen(instance)));
}


CGValue Intrinsic::llvmCodegen(LLVMBackend &instance) {
    switch (intrinsic_type) {
        case SIZEOF: {
            llvm::Type* val_type = args.at(0).llvmCodegen(instance).getRValue(instance)->getType();
            if (val_type->isPointerTy()) {
                return CGValue::rValue(instance.toLLVMInt(instance.getDataLayout().getPointerSize(0)));
            } if (val_type->isVoidTy()) {
                return CGValue::rValue(instance.toLLVMInt(0));
            } return CGValue::rValue(instance.toLLVMInt(instance.getDataLayout().getTypeSizeInBits(val_type) / 8));
        }
        case TYPEOF:
            return args.at(0).llvmCodegen(instance);
        case ADV_PTR: {
            assert(args.size() == 2);
            assert(args.at(0).expr_type->isPointerType());
            assert(args.at(1).expr_type->isIntegral());

            llvm::Value* ptr = args.at(0).llvmCodegen(instance).getRValue(instance);
            std::array operands{args.at(1).llvmCodegen(instance).getRValue(instance)};

            return CGValue::rValue(instance.Builder.CreateGEP(
                args.at(0).expr_type->getWrappedType()->llvmCodegen(instance),
                ptr, operands
                ));
        }

        default:
            throw std::runtime_error("Intrinsic::llvmCodegen: Unknown intrinsic");
    }
}


llvm::Value* LLVMBackend::castIfNecessary(Type* source_type, llvm::Value* subject) {
    // perform implicit-dereferencing, if applicable
    if (source_type->getTypeTag() == Type::REFERENCE) {
        auto referenced_type = dynamic_cast<ReferenceType*>(source_type)->of_type;
        if (!(getBoundTypeState()->getTypeTag() == Type::REFERENCE)) {
            subject = Builder.CreateLoad(
                referenced_type->llvmCodegen(*this),
                subject, "implicit_deref"
                );
        } source_type = referenced_type;
    }

    // if (source_type->isPointerType() && getBoundTypeState()->isIntegral()) {
    //     return Builder.CreatePtrToInt(subject, getBoundTypeState()->llvmCodegen(*this));
    // }

    if (getBoundTypeState() != source_type && source_type->getTypeTag() != Type::STRUCT) {
        if (getBoundTypeState()->isIntegral() || getBoundTypeState()->getTypeTag() == Type::BOOL) {
            // if the destination type is unsigned or if the source type is boolean
            // perform a zero-extension or truncation
            if (source_type->isIntegral()) {
                if (getBoundTypeState()->isUnsigned() || source_type->getTypeTag() == Type::BOOL) {
                    return Builder.CreateZExtOrTrunc(subject, getBoundLLVMType());
                } return Builder.CreateSExtOrTrunc(subject, getBoundLLVMType());
            }
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


CGValue Expression::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    assert(expr_type != nullptr);
    SET_BOUND_TYPE_STATE(expr_type);

    const auto val = expr->llvmCodegen(instance);
    return val;
}


CGValue Condition::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    const auto parent      = instance.Builder.GetInsertBlock()->getParent();
    const auto if_block    = llvm::BasicBlock::Create(instance.Context, "if", parent);
    const auto else_block  = llvm::BasicBlock::Create(instance.Context, "else", parent);
    const auto merge_block = llvm::BasicBlock::Create(instance.Context, "merge", parent);

    const auto if_cond = bool_expr.llvmCodegen(instance).getRValue(instance);
    instance.Builder.CreateCondBr(if_cond, if_block, else_block);

    {
        instance.Builder.SetInsertPoint(if_block);
        instance.codegenChildrenUntilRet(if_children, if_cond);

        // insert a jump to the merge block if the scope either doesn't end with a
        // terminator or is empty
        if (!instance.Builder.GetInsertBlock()->back().isTerminator() ||
            instance.Builder.GetInsertBlock()->empty())
            instance.Builder.CreateBr(merge_block);
    }

    {
        instance.Builder.SetInsertPoint(else_block);

        for (auto& [condition, children] : elif_children) {
            const auto cnd = condition.llvmCodegen(instance).getRValue(instance);

            auto elif_block = llvm::BasicBlock::Create(instance.Context, "elif", parent);
            auto next_elif  = llvm::BasicBlock::Create(instance.Context, "next_elif", parent);

            instance.Builder.CreateCondBr(cnd, elif_block, next_elif);
            instance.Builder.SetInsertPoint(elif_block);
            instance.codegenChildrenUntilRet(children, cnd);

            // insert a jump to the merge block if the scope either doesn't end with a
            // terminator or is empty
            if (!instance.Builder.GetInsertBlock()->back().isTerminator() ||
                instance.Builder.GetInsertBlock()->empty())
                instance.Builder.CreateBr(merge_block);

            instance.Builder.SetInsertPoint(next_elif);
        }

        instance.codegenChildrenUntilRet(else_children);

        // insert a jump to the merge block if the scope either doesn't end with a
        // terminator or is empty
        if (!instance.Builder.GetInsertBlock()->back().isTerminator() ||
            instance.Builder.GetInsertBlock()->empty())
            instance.Builder.CreateBr(merge_block);

        instance.Builder.SetInsertPoint(merge_block);
    }

    return {};
}


CGValue WhileLoop::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    const auto parent = instance.Builder.GetInsertBlock()->getParent();
    const auto last_inst = instance.Builder.GetInsertBlock()->getTerminator();

    const auto cond_block  = llvm::BasicBlock::Create(instance.Context, "while_cond", parent);
    const auto body_block  = llvm::BasicBlock::Create(instance.Context, "while_body", parent);
    const auto merge_block = llvm::BasicBlock::Create(instance.Context, "merge", parent);

    if (last_inst == nullptr)
        instance.Builder.CreateBr(cond_block);

    instance.Builder.SetInsertPoint(cond_block);
    const auto expr  = condition.llvmCodegen(instance).getRValue(instance);
    instance.Builder.CreateCondBr(expr, body_block, merge_block);

    instance.setLoopMetadata({.break_to = merge_block, .continue_to = cond_block});

    {
        instance.Builder.SetInsertPoint(body_block);
        instance.codegenChildrenUntilRet(children);

        if (!instance.Builder.GetInsertBlock()->back().isTerminator() ||
            instance.Builder.GetInsertBlock()->empty())
        instance.Builder.CreateBr(cond_block);
    }

    instance.restoreLoopMetadata();
    instance.Builder.SetInsertPoint(merge_block);
    return {nullptr, nullptr};
}


CGValue BreakStmt::llvmCodegen(LLVMBackend &instance) {
    instance.Builder.CreateBr(instance.getLoopMetadata().break_to);
    return {};
}

CGValue ContinueStmt::llvmCodegen(LLVMBackend &instance) {
    instance.Builder.CreateBr(instance.getLoopMetadata().continue_to);
    return {};
}

CGValue Struct::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    auto struct_sw_ty = instance.SymMan.lookupType(ident);
    assert(struct_sw_ty);

    instance.setManglingContext({.encapsulator = struct_sw_ty});
    for (auto& member : members) {
        if (member->getNodeType() == ND_FUNC) {
            member->llvmCodegen(instance);
        }
    } instance.restoreManglingContext();
    return {};
}


CGValue FuncCall::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    std::vector<llvm::Type*> paramTypes;

    assert(ident.getIdentInfo());
    auto fn_name = ident.getIdentInfo();

    llvm::Function* func = instance.LModule->getFunction(instance.mangleString(fn_name));
    if (!func) {
        [[maybe_unused]] auto fn = llvm::Function::Create(
            llvm::dyn_cast<llvm::FunctionType>(instance.SymMan.lookupType(ident.getIdentInfo())->llvmCodegen(instance)),
            llvm::GlobalValue::ExternalLinkage,
            instance.mangleString(fn_name),
            instance.LModule.get()
            );
        func = instance.LModule->getFunction(instance.mangleString(fn_name));
    }

    std::vector<llvm::Value*> arguments{};
    arguments.reserve(args.size() + 1);

    assert(ident.value);
    auto& entry = instance.SymMan.lookupDecl(ident.value);
    if ( entry.method_of && !entry.is_static) {
        // push the implicit instance pointer if the callee is a method and not static
        assert(instance.ComputedPtr);
        arguments.push_back(instance.ComputedPtr);  // push the ComputedPtr as an implicit argument
    }

    for (auto& item : args)
        arguments.push_back(item.llvmCodegen(instance).getRValue(instance));

    if (!func->getReturnType()->isVoidTy()) {
        Type* ret_type = nullptr;

        auto fn_type = dynamic_cast<FunctionType*>(instance.SymMan.lookupType(ident.value));
        ret_type = fn_type->ret_type;

        auto call = instance.Builder.CreateCall(func, arguments, ident.value->toString());

        return {call, call};
    }

    instance.Builder.CreateCall(func, arguments);
    return {};
}


CGValue Var::llvmCodegen(LLVMBackend &instance) {
    PRE_SETUP();
    assert(var_type.type != nullptr);

    llvm::Type* type = var_type.type->llvmCodegen(instance);
    assert(type != nullptr);

    llvm::Value* init = nullptr;

    auto linkage = (is_exported || is_extern) ?
        llvm::GlobalVariable::ExternalLinkage : llvm::GlobalVariable::InternalLinkage;


    if (!instance.isLocalScope()) {
        auto var_name = is_extern ? var_ident->toString() : instance.mangleString(var_ident);
        auto* var = new llvm::GlobalVariable(
                *instance.LModule, type, is_const, linkage,
                nullptr, var_name
                );

        if (initialized) {
            instance.BoundMemory = var;
            init = value.llvmCodegen(instance).getRValue(instance);
            const auto val = llvm::dyn_cast<llvm::Constant>(init);
            assert(val != nullptr);
            var->setInitializer(val);
            instance.BoundMemory = nullptr;
        }

        // handle references
        if (var_type.type->getTypeTag() == Type::REFERENCE) {
            instance.SymMan.lookupDecl(this->var_ident).llvm_value = instance.RefMemory;
        } else instance.SymMan.lookupDecl(this->var_ident).llvm_value = var;
    } else {
        llvm::AllocaInst* var_alloca = instance.Builder.CreateAlloca(
            type, nullptr, var_ident->toString());

        if (is_extern)
            return {};

        if (initialized) {
            // instance.BoundMemory = var_alloca;
            SET_BOUND_TYPE_STATE(var_type.type);
            init = value.llvmCodegen(instance).getRValue(instance);
            assert(init != nullptr);

            instance.Builder.CreateStore(init, var_alloca, is_volatile);

            instance.BoundMemory = nullptr;
        }

        // handle references
        if (var_type.type->getTypeTag() == Type::REFERENCE) {
            instance.SymMan.lookupDecl(this->var_ident).llvm_value = instance.RefMemory;
        } else instance.SymMan.lookupDecl(this->var_ident).llvm_value = var_alloca;
    }

    return {};
}
