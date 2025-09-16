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


LLVMBackend::LLVMBackend(Parser& parser)
    : LModule{
        std::make_unique<llvm::Module>(parser.m_FilePath.string(), Context)
    }
    , AST(std::move(parser.AST))
    , SymMan(parser.SymbolTable)
{
    m_LatestBoundType.emplace(nullptr);
    m_AssignmentLhsStack.emplace(false);
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

// TODO: mangling still needs to concatenate module UID into the string
std::string LLVMBackend::mangleString(IdentInfo* id) {
    auto decl_lookup = SymMan.lookupDecl(id);

    if (decl_lookup.node_loc) {
        // do not mangle `extern "C"` symbols
        if (decl_lookup.node_loc->isGlobal()) {
            if (dynamic_cast<GlobalNode*>(
                decl_lookup.node_loc
                )->extern_attributes.contains("C")) {
                return id->toString();
            }
        }
    }

    if (decl_lookup.is_method) {
        // this shall hold the encapsulating type
        auto type = getManglingContext().encapsulator;
        assert(type != nullptr);

        // unwrap the type if needed
        if (type->getTypeTag() == Type::REFERENCE)
            type = dynamic_cast<ReferenceType*>(type)->of_type;
        assert(type->getTypeTag() != Type::POINTER);

        return "__Sw_" + type->toString() + "_" + id->toString();
    } return "__Sw_" + id->toString();
}


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
    return ret;
}

llvm::Value* FloatLit::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    return llvm::ConstantFP::get(instance.getBoundLLVMType(), value);
}

llvm::Value* StrLit::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    return llvm::ConstantStruct::get(
        llvm::dyn_cast<llvm::StructType>(instance.getBoundLLVMType()), {
        llvm::ConstantDataArray::getString(instance.Context, value, false)
    });
}


llvm::Value* BoolLit::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    return llvm::ConstantInt::getBool(instance.Context, value);
}


/// Writes the array literal to 'BoundMemory' if not null, otherwise creates a temporary and
/// returns a load of it.
llvm::Value* ArrayLit::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    assert(!elements.empty());
    Type* element_type = elements.at(0).expr_type;
    Type* sw_arr_type  = instance.SymMan.getArrayType(element_type, elements.size());

    if (!instance.IsLocalScope) {
        if (instance.BoundMemory) {
            auto arr_type = llvm::ArrayType::get(element_type->llvmCodegen(instance), elements.size());

            std::vector<llvm::Constant*> val_array;
            val_array.reserve(elements.size());
            for (auto& elem : elements) {
                instance.setBoundTypeState(element_type);
                val_array.push_back(llvm::dyn_cast<llvm::Constant>(elem.llvmCodegen(instance)));
                instance.restoreBoundTypeState();
            }

            auto array_init = llvm::ConstantArray::get(arr_type, val_array);

            std::vector<llvm::Type*> members = {arr_type};

            auto const_struct = llvm::ConstantStruct::get(
                llvm::dyn_cast<llvm::StructType>(sw_arr_type->llvmCodegen(instance)),
                {array_init}
                );
            return const_struct;
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
            } instance.Builder.CreateStore(element.llvmCodegen(instance), ptr);
        }
        if (bound_mem_cache) instance.BoundMemory = bound_mem_cache;
        return nullptr;
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
        } instance.Builder.CreateStore(element.llvmCodegen(instance), ptr);
    }

    if (bound_mem_cache)
        instance.BoundMemory = bound_mem_cache;
    return instance.Builder.CreateLoad(instance.getBoundLLVMType(), tmp);
}


llvm::Value* Ident::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    const auto e = instance.SymMan.lookupDecl(this->value);

    if (instance.getAssignmentLhsState()) {
        assert(e.llvm_value != nullptr);
        return e.llvm_value;
    }

    if (!instance.IsLocalScope) {
        auto global_var = llvm::dyn_cast<llvm::GlobalVariable>(e.llvm_value);
        return global_var->getInitializer();
    }

    return e.is_param
    ? instance.castIfNecessary(e.swirl_type, e.llvm_value)
    : instance.castIfNecessary(
        e.swirl_type, instance.Builder.CreateLoad(
            e.swirl_type->llvmCodegen(instance), e.llvm_value));
}

llvm::Value* ImportNode::llvmCodegen([[maybe_unused]] LLVMBackend& instance) {
    return nullptr;
}


llvm::Value* Scope::llvmCodegen(LLVMBackend& instance) {
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
    } return nullptr;
}


llvm::Value* Function::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    std::recursive_mutex mtx;
    std::lock_guard guard(mtx);

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
        return func;
    }

    llvm::BasicBlock*   entry_bb = llvm::BasicBlock::Create(instance.Context, "entry", func);

    NewScope _(instance);
    instance.Builder.SetInsertPoint(entry_bb);

    for (unsigned int i = 0; i < params.size(); i++) {
        const auto p_name = params[i].var_ident;
        [[maybe_unused]] const auto param = func->getArg(i);
        // param->setName(p_name->toString());

        instance.SymMan.lookupDecl(p_name).llvm_value = func->getArg(i);
    }

    // for (const auto& child : this->children)
    //     child->codegen();

    codegenChildrenUntilRet(instance, children);
    if (!instance.Builder.GetInsertBlock()->back().isTerminator()
        || instance.Builder.GetInsertBlock()->empty())
        instance.Builder.CreateRetVoid();
    return func;
}


llvm::Value* ReturnStatement::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    if (value.expr) {
        llvm::Value* ret = value.llvmCodegen(instance);
        instance.Builder.CreateRet(ret);
        return nullptr;
    }

    instance.Builder.CreateRetVoid();
    return nullptr;
}


llvm::Value* Op::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    SET_BOUND_TYPE_STATE(common_type);

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
            Node* operand = operands.at(0).get();
            llvm::Value* ret = nullptr;

            Type* type = nullptr;
            if (operand->getNodeType() == ND_EXPR) {
                auto expr = dynamic_cast<Expression*>(operand);
                type = instance.fetchSwType(expr->expr);
            } else instance.fetchSwType(operands.at(0));

            // handle slice-creation for array types
            if (type->getTypeTag() == Type::ARRAY || type->getTypeTag() == Type::STR) {
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

                instance.setAssignmentLhsState(true);
                auto arr_instance_ptr = operand->llvmCodegen(instance);
                instance.restoreAssignmentLhsState();

                // calculate the pointer to the array
                auto element_ptr = instance.Builder.CreateStructGEP(
                    type->llvmCodegen(instance),
                    arr_instance_ptr,
                    0
                    );

                instance.Builder.CreateStore(element_ptr, ptr_field);
                instance.Builder.CreateStore(
                    instance.toLLVMInt(type->getAggregateSize()),
                    size_field
                    );

                return instance.getAssignmentLhsState() ?
                        llvm::dyn_cast<llvm::Value>(slice_instance) :
                        instance.Builder.CreateLoad(slice_llvm_ty, slice_instance);
            }

            if (operand->getNodeType() == ND_EXPR) {
                if (auto expr_unwrapped = operand->getExprValue().get();
                   expr_unwrapped->getNodeType() == ND_OP)
                {
                    if (auto op_node = dynamic_cast<Op*>(expr_unwrapped);
                        op_node->op_type == DOT ||
                        op_node->op_type == INDEXING_OP
                    ) {
                        instance.setAssignmentLhsState(true);
                        [[maybe_unused]] auto ptr = operand->llvmCodegen(instance);
                        instance.restoreAssignmentLhsState();
                        ret =  instance.ComputedPtr;
                    }
                    else throw;
                }

                else if (expr_unwrapped->getNodeType() == ND_IDENT) {
                    auto lookup = instance.SymMan.lookupDecl(operand->getIdentInfo());
                    ret = lookup.llvm_value;
                }
            }

            else {
                auto lookup = instance.SymMan.lookupDecl(operand->getIdentInfo());
                ret = lookup.llvm_value;
            }

            instance.RefMemory = ret;
            assert(ret);
            return ret;
        }

        case DIV: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);

            if (instance.getBoundTypeState()->isFloatingPoint()) {
                return instance.Builder.CreateFDiv(lhs, rhs);
            }
            if (instance.getBoundTypeState()->isUnsigned()) {
                return instance.Builder.CreateUDiv(lhs, rhs);
            }

            return instance.Builder.CreateSDiv(lhs, rhs);
        }

        case MOD: {
            llvm::Value* lhs = operands.at(0)->llvmCodegen(instance);
            llvm::Value* rhs = operands.at(1)->llvmCodegen(instance);
            
            // SemanticAnalysis ascertains that integral types are used
            if (instance.getBoundTypeState()->isUnsigned()) {
                return instance.Builder.CreateURem(lhs, rhs);
            }

            return instance.Builder.CreateSRem(lhs, rhs);
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
            throw;
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

            assert(lhs->getType() == rhs->getType());

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

            assert(lhs->getType() == rhs->getType());

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

            assert(lhs->getType() == rhs->getType());

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

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return instance.Builder.CreateFCmpOLE(lhs, rhs);
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return instance.Builder.CreateICmpULE(lhs, rhs);
            }

            return instance.Builder.CreateICmpSLE(lhs, rhs);
        }

        case ASSIGNMENT: {
            instance.setAssignmentLhsState(true);
            auto lhs = operands.at(0)->llvmCodegen(instance);
            instance.restoreAssignmentLhsState();
            instance.setBoundTypeState(common_type);
            instance.Builder.CreateStore(operands.at(1)->llvmCodegen(instance), lhs);
            instance.restoreBoundTypeState();
            return nullptr;
        }

        case INDEXING_OP: {
            auto underlying_type = instance.fetchSwType(operands.at(0))->getWrappedType();

            auto operand_llvm_ty = instance.fetchSwType(operands.at(0))->llvmCodegen(instance);

            instance.setAssignmentLhsState(true);
            auto arr_ptr = instance.Builder.CreateStructGEP(
                operand_llvm_ty, operands.at(0)->llvmCodegen(instance), 0);
            instance.restoreAssignmentLhsState();

            llvm::Value* second_op;
            instance.setBoundTypeState(instance.fetchSwType(operands.at(1)));
            if (instance.getAssignmentLhsState()) {
                instance.restoreAssignmentLhsState();
                second_op = operands.at(1)->llvmCodegen(instance);
                instance.setAssignmentLhsState(true);
            } else {
                second_op = operands.at(1)->llvmCodegen(instance);
            } instance.restoreBoundTypeState();

            auto element_ptr = instance.Builder.CreateGEP(
                operand_llvm_ty->getStructElementType(0),
                arr_ptr,
                {instance.toLLVMInt(0), second_op}
                );

            assert(element_ptr != nullptr);
            instance.ComputedPtr = element_ptr;
            if (instance.getAssignmentLhsState()) {
                return element_ptr;
            } return instance.Builder.CreateLoad(underlying_type->llvmCodegen(instance), element_ptr);
        }

        case DOT: {
            // when nested
            if (operands.at(0)->getNodeType() == ND_OP) {
                instance.setAssignmentLhsState(true);
                auto inst_ptr = operands.at(0)->llvmCodegen(instance);
                instance.restoreAssignmentLhsState();

                auto field_node = dynamic_cast<Ident*>(operands.at(1).get());
                auto struct_ty  = dynamic_cast<StructType*>(instance.StructFieldType);

                auto field_ptr = instance.Builder.CreateStructGEP(
                    instance.StructFieldType->llvmCodegen(instance),
                    inst_ptr,
                    static_cast<unsigned int>
                    (struct_ty->field_offsets.at(field_node->full_qualification.front()))
                );

                auto field_ty = instance.SymMan.lookupDecl(field_node->value).swirl_type;

                instance.StructFieldType = field_ty;
                instance.StructFieldPtr = field_ptr;
                instance.ComputedPtr = field_ptr;

                return instance.getAssignmentLhsState() ? field_ptr : instance.Builder.CreateLoad(
                    field_ty->llvmCodegen(instance),
                    field_ptr
                );
            // ---- * ---- * ---- * ---- * ---- //
            } else {  // otherwise...
                instance.setAssignmentLhsState(true);
                auto inst_ptr = operands.at(0)->llvmCodegen(instance);
                instance.restoreAssignmentLhsState();

                // in the special case when dot is using with pure-rvalues
                // e.g., functions returning struct instances
                if (!inst_ptr->getType()->isPointerTy()) {
                    auto tmp = instance.Builder.CreateAlloca(inst_ptr->getType());
                    instance.Builder.CreateStore(inst_ptr, tmp);
                    inst_ptr = tmp;
                }

                // fetch the instance's struct-type
                auto struct_sw_ty = instance.fetchSwType(operands.at(0));
                auto struct_ty = dynamic_cast<StructType*>(struct_sw_ty);

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
                    (struct_ty->field_offsets.at(field_node->full_qualification.front()))
                );

                auto field_type = instance.SymMan.lookupDecl(field_node->getIdentInfo()).swirl_type;

                instance.StructFieldPtr = field_ptr;
                instance.StructFieldType = field_type;
                instance.ComputedPtr = field_ptr;

                return instance.getAssignmentLhsState() ? field_ptr : instance.Builder.CreateLoad(
                    field_type->llvmCodegen(instance),
                    field_ptr
                );
            }
        }
        default: break;
    }

    using namespace std::string_view_literals;

    if (value.ends_with("=") && ("+-*/%~&^"sv.find(value.at(0)) != std::string::npos)) {
        auto op = std::make_unique<Op>(std::string_view{value.data(), 1}, 2);
        op->operands.push_back(std::move(operands.at(0)));
        op->operands.push_back(std::move(operands.at(1)));

        instance.setBoundTypeState(instance.fetchSwType(op->operands.at(1)));
        auto rhs = op->llvmCodegen(instance);
        instance.restoreBoundTypeState();

        instance.setAssignmentLhsState(true);
        auto lhs = op->operands.at(0)->llvmCodegen(instance);
        instance.restoreAssignmentLhsState();

        instance.Builder.CreateStore(rhs, lhs);
        return nullptr;
    }

    throw;
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

    if (getBoundTypeState() != source_type && source_type->getTypeTag() != Type::STRUCT) {
        if (getBoundTypeState()->isIntegral() || getBoundTypeState()->getTypeTag() == Type::BOOL) {
            // if the destination type is unsigned or if the source type is boolean
            // perform a zero-extension or truncation
            if (getBoundTypeState()->isUnsigned() || source_type->getTypeTag() == Type::BOOL) {
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
    PRE_SETUP();
    assert(expr_type != nullptr);

    instance.setBoundTypeState(expr_type);
    const auto val = expr->llvmCodegen(instance);
    instance.restoreBoundTypeState();

    return val;
}


llvm::Value* Assignment::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    instance.setAssignmentLhsState(true);
    const auto lv = l_value.llvmCodegen(instance);
    instance.restoreAssignmentLhsState();

    instance.Builder.CreateStore(r_value.llvmCodegen(instance), lv);
    return nullptr;
}


llvm::Value* Condition::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
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

        // insert a jump to the merge block if the scope either doesn't end with a
        // terminator or is empty
        if (!instance.Builder.GetInsertBlock()->back().isTerminator() ||
            instance.Builder.GetInsertBlock()->empty())
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

            // insert a jump to the merge block if the scope either doesn't end with a
            // terminator or is empty
            if (!instance.Builder.GetInsertBlock()->back().isTerminator() ||
                instance.Builder.GetInsertBlock()->empty())
                instance.Builder.CreateBr(merge_block);

            instance.Builder.SetInsertPoint(next_elif);
        }

        codegenChildrenUntilRet(instance, else_children);

        // insert a jump to the merge block if the scope either doesn't end with a
        // terminator or is empty
        if (!instance.Builder.GetInsertBlock()->back().isTerminator() ||
            instance.Builder.GetInsertBlock()->empty())
            instance.Builder.CreateBr(merge_block);

        instance.Builder.SetInsertPoint(merge_block);
    }

    return nullptr;
}


llvm::Value* WhileLoop::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
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


llvm::Value* Struct::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    auto struct_sw_ty = instance.SymMan.lookupType(ident);
    assert(struct_sw_ty);

    instance.setManglingContext({.encapsulator = struct_sw_ty});
    for (auto& member : members) {
        if (member->getNodeType() == ND_FUNC) {
            member->llvmCodegen(instance);
        }
    } instance.restoreManglingContext();
    return nullptr;
}


llvm::Value* FuncCall::llvmCodegen(LLVMBackend& instance) {
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
    if (instance.SymMan.lookupDecl(ident.value).is_method) {  // is the func call a method call?
        assert(instance.ComputedPtr);
        arguments.push_back(instance.ComputedPtr);  // push the ComputedPtr as an implicit argument
    }

    for (auto& item : args)
        arguments.push_back(item.llvmCodegen(instance));

    if (!func->getReturnType()->isVoidTy()) {
        Type* ret_type = nullptr;

        auto fn_type = dynamic_cast<FunctionType*>(instance.SymMan.lookupType(ident.value));
        ret_type = fn_type->ret_type;

        return instance.castIfNecessary(
            ret_type,
            instance.Builder.CreateCall(
                func,
                arguments,
                ident.value->toString()
                )
            );
    }

    instance.Builder.CreateCall(func, arguments);
    return nullptr;
}


llvm::Value* Var::llvmCodegen(LLVMBackend& instance) {
    PRE_SETUP();
    assert(var_type != nullptr);
    // if (is_comptime) { return nullptr; }

    llvm::Type* type = var_type->llvmCodegen(instance);

    llvm::Value* init = nullptr;

    auto linkage = (is_exported || is_extern) ?
        llvm::GlobalVariable::ExternalLinkage : llvm::GlobalVariable::InternalLinkage;


    if (!instance.IsLocalScope) {
        auto var_name = is_extern ? var_ident->toString() : instance.mangleString(var_ident);
        auto* var = new llvm::GlobalVariable(
                *instance.LModule, type, is_const, linkage,
                nullptr, var_name
                );

        if (initialized) {
            instance.BoundMemory = var;
            init = value.llvmCodegen(instance);
            const auto val = llvm::dyn_cast<llvm::Constant>(init);
            assert(val != nullptr);
            var->setInitializer(val);
            instance.BoundMemory = nullptr;
        }

        // handle references
        if (var_type->getTypeTag() == Type::REFERENCE) {
            instance.SymMan.lookupDecl(this->var_ident).llvm_value = instance.RefMemory;
        } else instance.SymMan.lookupDecl(this->var_ident).llvm_value = var;
    } else {
        llvm::AllocaInst* var_alloca = instance.Builder.CreateAlloca(type, nullptr, var_ident->toString());
        if (is_extern) return var_alloca;

        if (initialized) {
            instance.BoundMemory = var_alloca;
            init = value.llvmCodegen(instance);
            if (init) {
                instance.Builder.CreateStore(init, var_alloca, is_volatile);
            }

            instance.BoundMemory = nullptr;
        }

        // handle references
        if (var_type->getTypeTag() == Type::REFERENCE) {
            instance.SymMan.lookupDecl(this->var_ident).llvm_value = instance.RefMemory;
        } else instance.SymMan.lookupDecl(this->var_ident).llvm_value = var_alloca;
    }

    return nullptr;
}
