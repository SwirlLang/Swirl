#include "backend/LLVMBackend.h"
#include "CompilerInst.h"
#include "managers/ModuleManager.h"
#include "parser/Parser.h"
#include "types/SwTypes.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>


/// Creates and returns a CGValue out of n lvalue
CGValue CGValue::lValue(llvm::Value* lvalue) {
    return {lvalue, nullptr};
}

/// Creates and returns a CGValue out of a rvalue
CGValue CGValue::rValue(llvm::Value* rvalue) {
    return {nullptr, rvalue};
}

/// Returns the held lvalue
llvm::Value* CGValue::getLValue() const {
    if (!m_LValue) {
        assert(m_RValue);
        assert(m_RValue->getType()->isPointerTy());
        return m_RValue;
    } return m_LValue;
}

bool CGValue::isLValue() const {
    if (m_LValue || (m_RValue && m_RValue->getType()->isPointerTy())) {
        return true;
    } return false;
}


/// If there's no held rvalue, creates and returns a load of the held lvalue, returns the held
/// rvalue otherwise.
llvm::Value* CGValue::getRValue(LLVMBackend& instance, const SwContext& context) const {
    if (!m_RValue) {
        assert(m_LValue);
        return instance.Builder.CreateLoad(
            instance.codegen(context.bound_type, context), m_LValue);
    } return m_RValue;
}

LLVMBackend::LLVMBackend(Parser& parser)
    : ModuleManager(parser.ModuleMap)
      , SymMan(parser.SymbolTable)
      , LModule{
          std::make_unique<llvm::Module>(
              ModuleManager.getModuleUID(parser.m_FileHandle->getPath()),
              LLVMContext)
      }
{
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
            llvm::Triple(CompilerInst::TargetTriple), "generic", "", options, reloc_model
        );
    });

    assert(TargetMachine);
    LModule->setDataLayout(TargetMachine->createDataLayout());
    LModule->setTargetTriple(llvm::Triple(CompilerInst::TargetTriple));
}


std::string LLVMBackend::mangleString(IdentInfo* id) const {
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
               '_' + ModuleManager.getModuleUID(id->getModuleFileHandle()->getPath()) + "_" + id->toString();
    } return "__Sw_" + ModuleManager.getModuleUID(id->getModuleFileHandle()->getPath()) + '_' + id->toString();
}


llvm::Value* LLVMBackend::castIfNecessary(Type* source_type, llvm::Value* subject, const SwContext& context) {
    // perform implicit-dereferencing, if applicable
    if (source_type->getTypeTag() == Type::REFERENCE) {
        auto referenced_type = source_type->to<ReferenceType>()->of_type;
        if (context.bound_type->getTypeTag() != Type::REFERENCE) {
            subject = Builder.CreateLoad(
                codegen(referenced_type, context),
                subject, "implicit_deref"
            );
        } source_type = referenced_type;
    }

    // if (source_type->isPointerType() && getBoundTypeState()->isIntegral()) {
    //     return Builder.CreatePtrToInt(subject, getBoundTypeState()->llvmCodegen(*this));
    // }

    if (context.bound_type != source_type && source_type->getTypeTag() != Type::STRUCT) {
        if (context.bound_type->isIntegral() || context.bound_type->getTypeTag() == Type::BOOL) {
            // if the destination type is unsigned or if the source type is boolean
            // perform a zero-extension or truncation
            if (source_type->isIntegral()) {
                if (context.bound_type->isUnsigned() || source_type->getTypeTag() == Type::BOOL) {
                    return Builder.CreateZExtOrTrunc(subject, codegen(context.bound_type, context));
                } return Builder.CreateSExtOrTrunc(subject, codegen(context.bound_type, context));
            }
        }

        if (context.bound_type->isFloatingPoint()) {
            if (source_type->isFloatingPoint()) {
                return Builder.CreateFPCast(subject, codegen(context.bound_type, context));
            }

            if (source_type->isIntegral()) {
                if (source_type->isUnsigned()) {
                    return Builder.CreateUIToFP(subject, codegen(context.bound_type, context));
                } return Builder.CreateSIToFP(subject, codegen(context.bound_type, context));
            }
        }
    } return subject;
}


void LLVMBackend::codegenChildrenUntilRet(const NodesVec& children, const SwContext& context, llvm::Value* condition) {
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
                codegen(child.get(), context);
                return;
            case ND_INVALID:
                continue;
            default:
                codegen(child.get(), context);
        }
    }
}


CGValue LLVMBackend::llvmCodegen(Expression* node, SwContext context) {
    assert(node->expr_type != nullptr);
    context.bound_type = node->expr_type;
    const auto val = codegen(node->expr, context);
    return val;
}


CGValue LLVMBackend::llvmCodegen(const IntLit* node, const SwContext& context) {
    llvm::Value* ret = nullptr;

    assert(context.bound_type);
    if (context.bound_type->isIntegral()) {
        auto int_type = llvm::dyn_cast<llvm::IntegerType>(codegen(context.bound_type, context));
        assert(int_type);

        if (node->value.starts_with("0x"))
            ret = llvm::ConstantInt::get(int_type, node->value.substr(2), 16);
        else if (node->value.starts_with("0o"))
            ret = llvm::ConstantInt::get(int_type, node->value.substr(2), 8);
        else if (node->value.starts_with("0b"))
            ret = llvm::ConstantInt::get(int_type, node->value.substr(2), 2);
        else ret = llvm::ConstantInt::get(int_type, node->value, 10);
    }

    else if (context.bound_type->isFloatingPoint()) {
        ret = llvm::ConstantFP::get(codegen(context.bound_type, context), node->value);
    } else if (context.bound_type->getTypeTag() == Type::BOOL) {
        ret = llvm::ConstantInt::get(llvm::Type::getInt32Ty(LLVMContext), std::to_string(toInteger(node->value)), 10);
    }

    else {
        throw std::runtime_error(std::format("Fatal: IntLit::llvmCodegen called but instance is neither in "
                                             "integral nor FP state. State: `{}`", context.bound_type->toString()));
    }
    return CGValue::rValue(ret);
}


CGValue LLVMBackend::llvmCodegen(const FloatLit* node, const SwContext& context) {
    return CGValue::rValue(llvm::ConstantFP::get(codegen(context.bound_type, context), node->value));
}


CGValue LLVMBackend::llvmCodegen(Op* node, SwContext context) {
    assert(node->common_type);
    context.bound_type = node->common_type;


    switch (node->op_type) {
        case Op::UNARY_ADD:
            return codegen(node->operands.back().get(), context);

        case Op::UNARY_SUB:
            return CGValue::rValue(
                Builder.CreateNeg(codegen(node->operands.back(), context).getRValue(*this, context)));

        case Op::BINARY_ADD: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            if (context.bound_type->isIntegral()) {
                return CGValue::rValue(Builder.CreateAdd(lhs, rhs));
            }

            assert(lhs->getType()->isFloatingPointTy() && rhs->getType()->isFloatingPointTy());
            return CGValue::rValue(Builder.CreateFAdd(lhs, rhs));
        }

        case Op::BINARY_SUB: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            if (context.bound_type->isIntegral()) {
                return CGValue::rValue(Builder.CreateSub(lhs, rhs));
            }
            return CGValue::rValue(Builder.CreateFSub(lhs, rhs));
        }

        case Op::MUL: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            if (context.bound_type->isIntegral()) {
                return CGValue::rValue(Builder.CreateMul(lhs, rhs));
            }

            return CGValue::rValue(Builder.CreateFMul(lhs, rhs));
        }

        case Op::DEREFERENCE: {
            // preserve the l-value of the operand
            auto operand = codegen(node->getLHS(), context);
            return CGValue::lValue(operand.getRValue(*this, context));
        }

        case Op::ADDRESS_TAKING: {
            auto operand = node->getLHS()->getWrappedNodeOrInstance();
            Type* type = fetchSwType(operand);

            // handle slice-creation for array types
            if (type->getTypeTag() == Type::ARRAY && !node->common_type->isPointerType()) {
                auto slice_type = SymMan.getSliceType(type->getWrappedType(), node->is_mutable);

                auto slice_llvm_ty = codegen(slice_type, context);

                auto slice_instance = Builder.CreateAlloca(slice_llvm_ty);
                auto ptr_field = Builder.CreateStructGEP(
                    slice_llvm_ty,
                    slice_instance,
                    0
                );

                auto size_field = Builder.CreateStructGEP(
                    slice_llvm_ty,
                    slice_instance,
                    1
                );

                auto arr_instance_ptr = codegen(operand, context);

                // calculate the pointer to the array
                auto element_ptr = Builder.CreateStructGEP(
                    codegen(type, context),
                    arr_instance_ptr.getLValue(),
                    0
                );

                Builder.CreateStore(element_ptr, ptr_field);
                Builder.CreateStore(
                    toLLVMInt(type->getAggregateSize()),
                    size_field
                );

                return CGValue::rValue(llvm::dyn_cast<llvm::Value>(slice_instance));
            }

            if (operand->getNodeType() == ND_IDENT) {
                auto& tab_entry = SymMan.lookupDecl(operand->getIdentInfo());
                auto op_address = tab_entry.llvm_value;
                assert(op_address->getType()->isPointerTy());

                RefMemory = op_address;

                return CGValue{op_address, op_address};

            } throw;
        }

        case Op::DIV: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            if (context.bound_type->isFloatingPoint()) {
                return CGValue::rValue(Builder.CreateFDiv(lhs, rhs));
            }
            if (context.bound_type->isUnsigned()) {
                return CGValue::rValue(Builder.CreateUDiv(lhs, rhs));
            }

            return CGValue::rValue(Builder.CreateSDiv(lhs, rhs));
        }

        case Op::MOD: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            // SemanticAnalysis ascertains that integral types are used
            if (context.bound_type->isUnsigned()) {
                return CGValue::rValue(Builder.CreateURem(lhs, rhs));
            }

            return CGValue::rValue(Builder.CreateSRem(lhs, rhs));
        }

        case Op::CAST_OP: {
            assert(node->operands.at(1)->getNodeType() == ND_TYPE);
            context.bound_type = node->operands.at(1)->getSwType();
            return CGValue::rValue(
                castIfNecessary(
                    fetchSwType(node->operands.at(0).get()),
                    codegen(node->getLHS(), context).getRValue(*this, context), context));
        }

        case Op::LOGICAL_EQUAL: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            assert(lhs->getType() == rhs->getType());

            if (lhs->getType()->isFloatingPointTy()) {
                return CGValue::rValue(Builder.CreateFCmpOEQ(lhs, rhs));
            }

            if (lhs->getType()->isIntegerTy()) {
                return CGValue::rValue(Builder.CreateICmpEQ(lhs, rhs));
            }
            throw;
        }

        case Op::LOGICAL_NOTEQUAL: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            assert(lhs->getType() == rhs->getType());

            if (lhs->getType()->isFloatingPointTy()) {
                return CGValue::rValue(Builder.CreateFCmpONE(lhs, rhs));
            }

            if (lhs->getType()->isIntegerTy()) {
                return CGValue::rValue(Builder.CreateICmpNE(lhs, rhs));
            }
            throw;
        }

        case Op::LOGICAL_AND: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            return CGValue::rValue(Builder.CreateLogicalAnd(lhs, rhs));
        }

        case Op::LOGICAL_NOT: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            return CGValue::rValue(Builder.CreateNot(lhs));
        }

        case Op::LOGICAL_OR: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            return CGValue::rValue(Builder.CreateLogicalOr(lhs, rhs));
        }

        case Op::GREATER_THAN: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            auto lhs_type = fetchSwType(node->operands.at(0));
            auto rhs_type = fetchSwType(node->operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(Builder.CreateFCmpOGT(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(Builder.CreateICmpUGT(lhs, rhs));
            }

            return CGValue::rValue(Builder.CreateICmpSGT(lhs, rhs));
        }

        case Op::GREATER_THAN_OR_EQUAL: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            auto lhs_type = fetchSwType(node->operands.at(0));
            auto rhs_type = fetchSwType(node->operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(Builder.CreateFCmpOGE(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(Builder.CreateICmpUGE(lhs, rhs));
            }

            return CGValue::rValue(Builder.CreateICmpSGE(lhs, rhs));
        }

        case Op::LESS_THAN: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            auto lhs_type = fetchSwType(node->operands.at(0));
            auto rhs_type = fetchSwType(node->operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(Builder.CreateFCmpOLT(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(Builder.CreateICmpULT(lhs, rhs));
            }

            return CGValue::rValue(Builder.CreateICmpSLT(lhs, rhs));
        }

        case Op::LESS_THAN_OR_EQUAL: {
            llvm::Value* lhs = codegen(node->getLHS(), context).getRValue(*this, context);
            llvm::Value* rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            auto lhs_type = fetchSwType(node->operands.at(0));
            auto rhs_type = fetchSwType(node->operands.at(1));

            assert(lhs->getType() == rhs->getType());

            if (lhs_type->isFloatingPoint() || rhs_type->isFloatingPoint()) {
                return CGValue::rValue(Builder.CreateFCmpOLE(lhs, rhs));
            }

            if (lhs_type->isUnsigned() && rhs_type->isUnsigned()) {
                return CGValue::rValue(Builder.CreateICmpULE(lhs, rhs));
            }

            return CGValue::rValue(Builder.CreateICmpSLE(lhs, rhs));
        }

        case Op::ASSIGNMENT: {
            auto lhs = codegen(node->getLHS(), context).getLValue();
            context.bound_type = node->common_type;
            auto rhs = codegen(node->getRHS(), context).getRValue(*this, context);

            assert(lhs->getType()->isPointerTy());
            Builder.CreateStore(rhs, lhs);
            return {};
        }

        case Op::INDEXING_OP: {
            auto operand_llvm_ty = codegen(fetchSwType(node->operands.at(0)), context);

            auto arr_ptr = Builder.CreateStructGEP(
                operand_llvm_ty, codegen(node->getLHS(), context).getLValue(), 0);

            context.bound_type = fetchSwType(node->operands.at(1));
            llvm::Value* second_op = codegen(node->operands.at(1), context).getRValue(*this, context);

            auto element_ptr = Builder.CreateGEP(
                operand_llvm_ty->getStructElementType(0),
                arr_ptr,
                {toLLVMInt(0), second_op}
            );

            assert(element_ptr != nullptr);
            ComputedPtr = element_ptr;
            return CGValue::lValue(element_ptr);
        }

        case Op::DOT: {
            // when nested
            if (node->operands.at(0)->getNodeType() == ND_OP) {
                const auto inst_ptr = codegen(node->operands.at(0), context).getLValue();

                const auto field_node = node->operands.at(1)->to<Ident>();
                const auto struct_ty  = StructFieldType->to<StructType>();

                auto field_ptr = Builder.CreateStructGEP(
                    codegen(StructFieldType, context),
                    inst_ptr,
                    static_cast<unsigned int>
                    (struct_ty->field_offsets.at(field_node->full_qualification.front().name))
                );

                auto field_ty = SymMan.lookupDecl(field_node->value).swirl_type;

                StructFieldType = field_ty;
                StructFieldPtr  = field_ptr;
                ComputedPtr     = field_ptr;

                return CGValue::lValue(field_ptr);
            }

            // ---- * ---- * ---- * ---- * ---- //
            // otherwise...
            auto operand = codegen(node->operands.at(0), context);

            // fetch the instance's struct-type
            auto struct_sw_ty = fetchSwType(node->operands.at(0));
            auto struct_ty = struct_sw_ty->getWrappedTypeOrInstance()->to<StructType>();

            llvm::Value* inst_ptr;
            if (operand.isLValue()) {
                inst_ptr = operand.getLValue();
            } else {
                // when the LHS isn't an LValue
                inst_ptr = Builder.CreateAlloca(codegen(struct_ty, context));
                Builder.CreateStore(operand.getRValue(*this, context), inst_ptr);
            }

            // in the special case when dot is using with pure-rvalues
            // e.g., functions returning struct instances
            if (!inst_ptr->getType()->isPointerTy()) {
                auto tmp = Builder.CreateAlloca(inst_ptr->getType());
                Builder.CreateStore(inst_ptr, tmp);
                inst_ptr = tmp;
            }

            // handle the special case of methods, lower them into regular function calls
            if (node->operands.at(1)->getNodeType() == ND_CALL) {
                ComputedPtr = inst_ptr;  // set ComputedPtr for the FuncCall node to grab it
                auto ret = codegen(node->operands.at(1), context);
                return ret;
            }

            assert(struct_ty != nullptr);
            auto field_node = node->operands.at(1)->to<Ident>();
            auto field_ptr = Builder.CreateStructGEP(
                codegen(struct_ty, context),
                inst_ptr,
                static_cast<unsigned int>
                (struct_ty->field_offsets.at(field_node->full_qualification.front().name))
            );

            // in the case of this operator, the node->common_type is supposed to be the type of the field
            // being accessed, not an actual common type between node->operands
            auto field_type = node->common_type;

            StructFieldPtr = field_ptr;
            StructFieldType = field_type;
            ComputedPtr = field_ptr;

            return CGValue::lValue(field_ptr);
        }
        default: break;
    }

    using namespace std::string_view_literals;

    if (node->value.ends_with("=") && ("+-*/%~&^"sv.find(node->value.at(0)) != std::string::npos)) {
        auto op = std::make_unique<Op>(std::string_view{node->value.data(), 1}, 2);
        op->operands.push_back(std::move(node->operands.at(0)));
        op->operands.push_back(std::move(node->operands.at(1)));
        op->common_type = node->common_type;

        auto new_ctx = context;
        new_ctx.bound_type = fetchSwType(op->operands.at(1));

        auto rhs = codegen(op.get(), new_ctx).getRValue(*this, new_ctx);
        auto lhs = codegen(op->operands.at(0), context).getLValue();

        Builder.CreateStore(rhs, lhs);
        return {};
    }

    throw;
}


CGValue LLVMBackend::llvmCodegen(const StrLit* node, const SwContext& context) {
    auto ptr = Builder.CreateGlobalString(
        node->value, "", 0, getLLVMModule(), false);

    return CGValue::rValue(llvm::ConstantStruct::get(
        llvm::dyn_cast<llvm::StructType>(codegen(&GlobalTypeStr, context)), {
            ptr,
            llvm::dyn_cast<llvm::Constant>(toLLVMInt(node->value.size()))
        }));
}


CGValue LLVMBackend::llvmCodegen(Ident* node, const SwContext& context) {
    const auto e = SymMan.lookupDecl(node->value);

    if (node->value->isFictitious()) {
        auto enum_node = SymMan.getFictitiousIDValue(node->value);
        return {nullptr, llvm::ConstantInt::get(
            codegen(enum_node->enum_type->type, context),
            enum_node->entries.at(node->full_qualification.back().name))};
    }

    if (!isLocalScope()) {
        auto global_var = llvm::dyn_cast<llvm::GlobalVariable>(e.llvm_value);
        return CGValue::rValue(global_var->getInitializer());
    }

    // parameters are passed by value
    if (e.is_param) {
        return CGValue::rValue(e.llvm_value);
    }

    return {e.llvm_value, castIfNecessary(
        e.swirl_type, Builder.CreateLoad(
            codegen(e.swirl_type, context), e.llvm_value), context)};
}


CGValue LLVMBackend::llvmCodegen(Function* node, const SwContext& context) {
    if (!node->generic_params.empty())
        return {};

    const auto fn_sw_type = SymMan.lookupType(node->ident)->to<FunctionType>();

    const auto name = node->is_extern || node->ident->toString() == "main" ?
                          node->ident->toString() :
                          mangleString(node->ident);

    const auto linkage = (node->is_exported || node->is_extern || node->ident->toString() == "main"
                         ) ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage;

    auto*               fn_type  = llvm::dyn_cast<llvm::FunctionType>(codegen(fn_sw_type, context));
    llvm::Function*     func     = llvm::Function::Create(fn_type, linkage, name, LModule.get());

    SW_LLVM_SET_CURRENT_PARENT(func);

    if (node->is_extern) {
        if (node->extern_attributes == "C")
            func->setCallingConv(llvm::CallingConv::C);
        return {};
    }

    llvm::BasicBlock*   entry_bb = llvm::BasicBlock::Create(LLVMContext, "entry", func);

    Builder.SetInsertPoint(entry_bb);

    for (unsigned int i = 0; i < node->params.size(); i++) {
        const auto p_name = node->params[i].var_ident;
        [[maybe_unused]] const auto param = func->getArg(i);
        // param->setName(p_name->toString());

        SymMan.lookupDecl(p_name).llvm_value = func->getArg(i);
    }

    codegenChildrenUntilRet(node->children, context);
    if (!Builder.GetInsertBlock()->back().isTerminator()
        || Builder.GetInsertBlock()->empty())
        Builder.CreateRetVoid();

    return {};
}


CGValue LLVMBackend::llvmCodegen(Condition* node, const SwContext& context) {
    const auto parent      = Builder.GetInsertBlock()->getParent();
    const auto if_block    = llvm::BasicBlock::Create(LLVMContext, "if", parent);
    const auto else_block  = llvm::BasicBlock::Create(LLVMContext, "else", parent);
    const auto merge_block = llvm::BasicBlock::Create(LLVMContext, "merge", parent);

    const auto if_cond = codegen(&node->bool_expr, context).getRValue(*this, context);
    Builder.CreateCondBr(if_cond, if_block, else_block);

    {
        Builder.SetInsertPoint(if_block);
        codegenChildrenUntilRet(node->if_children, context, if_cond);

        // insert a jump to the merge block if the scope either doesn't end with a
        // terminator or is empty
        if (!Builder.GetInsertBlock()->back().isTerminator() ||
            Builder.GetInsertBlock()->empty())
            Builder.CreateBr(merge_block);
    }

    {
        Builder.SetInsertPoint(else_block);

        for (auto& [condition, children] : node->elif_children) {
            const auto cnd = codegen(&condition, context).getRValue(*this, context);

            auto elif_block = llvm::BasicBlock::Create(LLVMContext, "elif", parent);
            auto next_elif  = llvm::BasicBlock::Create(LLVMContext, "next_elif", parent);

            Builder.CreateCondBr(cnd, elif_block, next_elif);
            Builder.SetInsertPoint(elif_block);
            codegenChildrenUntilRet(children, context, cnd);

            // insert a jump to the merge block if the scope either doesn't end with a
            // terminator or is empty
            if (!Builder.GetInsertBlock()->back().isTerminator() ||
                Builder.GetInsertBlock()->empty())
                Builder.CreateBr(merge_block);

            Builder.SetInsertPoint(next_elif);
        }

        codegenChildrenUntilRet(node->else_children, context);

        // insert a jump to the merge block if the scope either doesn't end with a
        // terminator or is empty
        if (!Builder.GetInsertBlock()->back().isTerminator() ||
            Builder.GetInsertBlock()->empty())
            Builder.CreateBr(merge_block);

        Builder.SetInsertPoint(merge_block);
    }

    return {};
}


CGValue LLVMBackend::llvmCodegen(WhileLoop* node, SwContext context) {
    const auto parent = Builder.GetInsertBlock()->getParent();
    const auto last_inst = Builder.GetInsertBlock()->getTerminator();

    const auto cond_block  = llvm::BasicBlock::Create(LLVMContext, "while_cond", parent);
    const auto body_block  = llvm::BasicBlock::Create(LLVMContext, "while_body", parent);
    const auto merge_block = llvm::BasicBlock::Create(LLVMContext, "merge", parent);

    if (last_inst == nullptr)
        Builder.CreateBr(cond_block);

    Builder.SetInsertPoint(cond_block);
    const auto expr  = codegen(&node->condition, context).getRValue(*this, context);
    Builder.CreateCondBr(expr, body_block, merge_block);


    context.break_to = merge_block;
    context.continue_to = cond_block;

    {
        Builder.SetInsertPoint(body_block);
        codegenChildrenUntilRet(node->children, context);

        if (!Builder.GetInsertBlock()->back().isTerminator() ||
            Builder.GetInsertBlock()->empty())
            Builder.CreateBr(cond_block);
    }

    Builder.SetInsertPoint(merge_block);
    return {};
}


/// Writes the array literal to 'BoundMemory' if not null, otherwise creates a temporary and
/// returns a load of it.
CGValue LLVMBackend::llvmCodegen(ArrayLit* node, const SwContext& context) {
    assert(!node->elements.empty());
    Type* element_type = node->elements.at(0).expr_type;
    Type* sw_arr_type  = SymMan.getArrayType(element_type, node->elements.size());

    if (!isLocalScope()) {
        if (context.bound_memory) {
            auto arr_type = llvm::ArrayType::get(codegen(element_type, context), node->elements.size());

            std::vector<llvm::Constant*> val_array;
            val_array.reserve(node->elements.size());
            for (auto& elem : node->elements) {
                auto new_ctx = context;
                new_ctx.bound_type = element_type;
                val_array.push_back(llvm::dyn_cast<llvm::Constant>(
                    codegen(&elem, new_ctx).getRValue(*this, new_ctx)));
            }

            auto array_init = llvm::ConstantArray::get(arr_type, val_array);

            std::vector<llvm::Type*> members = {arr_type};

            auto const_struct = llvm::ConstantStruct::get(
                llvm::dyn_cast<llvm::StructType>(codegen(sw_arr_type, context)),
                {array_init}
            );
            return CGValue::rValue(const_struct);
        }
    }

    // the array-type which is enclosed within a struct
    assert(codegen(context.bound_type, context)->isStructTy());
    llvm::Type*  arr_type = codegen(context.bound_type, context)->getStructElementType(0);
    llvm::Value* ptr  = nullptr;  // the to-be-calculated pointer to where the array is supposed to be written

    // if this flag is set, the literal shall be written to the storage that it represents
    if (context.bound_memory) {
        // set llvm_value = array member
        auto base_ptr = Builder.CreateStructGEP(codegen(context.bound_type, context), context.bound_memory, 0);

        for (auto [i, element] : llvm::enumerate(node->elements)) {
            ptr = Builder.CreateGEP(arr_type, base_ptr, {toLLVMInt(0), toLLVMInt(i)});
            if (element.expr_type->getTypeTag() == Type::ARRAY) {
                auto new_ctx = context;
                new_ctx.bound_memory = ptr;
                new_ctx.bound_type = element_type;
                codegen(&element, new_ctx);
                continue;
            } Builder.CreateStore(codegen(&element, context).getRValue(*this, context), ptr);
        }

        return {};
    }

    auto tmp = Builder.CreateAlloca(codegen(context.bound_type, context));
    auto base_ptr = Builder.CreateStructGEP(codegen(context.bound_type, context), tmp, 0);

    for (auto [i, element] : llvm::enumerate(node->elements)) {
        ptr = Builder.CreateGEP(arr_type, base_ptr, {toLLVMInt(0), toLLVMInt(i)});
        if (element.expr_type->getTypeTag() == Type::ARRAY) {
            auto new_ctx = context;
            new_ctx.bound_memory = ptr;
            new_ctx.bound_type = element_type;
            codegen(&element, new_ctx);
            continue;
        } Builder.CreateStore(codegen(&element, context).getRValue(*this, context), ptr);
    }

    auto tmp_load = Builder.CreateLoad(codegen(context.bound_type, context), tmp);
    assert(tmp_load);
    return CGValue::rValue(tmp_load);
}


CGValue LLVMBackend::llvmCodegen(const TypeWrapper* node, const SwContext& context) {
    return CGValue::rValue(llvm::UndefValue::get(codegen(node->type, context)));
}


CGValue LLVMBackend::llvmCodegen(BoolLit* node, SwContext context) {
    return CGValue::rValue(llvm::ConstantInt::getBool(LLVMContext, node->value));
}


CGValue LLVMBackend::llvmCodegen(Scope* node, const SwContext& context) {
    assert(getCurrentParent() != nullptr);

    // create a new basic block if:
    // - no active basic blocks exist, or
    // - the last active basic block's last instruction is a terminator
    if (
        !Builder.GetInsertBlock() ||
        Builder.GetInsertBlock()->back().isTerminator()
    ) {

        auto bb = llvm::BasicBlock::Create(
            LLVMContext,
            "scope_0",
            getCurrentParent()
        );

        Builder.SetInsertPoint(bb);
    }

    for (auto& child : node->children) {
        if (child->getNodeType() == ND_INVALID) {
            continue;
        } codegen(child.get(), context);
    } return {};
}


CGValue LLVMBackend::llvmCodegen(BreakStmt* node, const SwContext& context) {
    assert(context.break_to);
    Builder.CreateBr(context.break_to);
    return {};
}


CGValue LLVMBackend::llvmCodegen(ContinueStmt* node, const SwContext& context) {
    assert(context.continue_to);
    Builder.CreateBr(context.continue_to);
    return {};
}

CGValue LLVMBackend::llvmCodegen(Intrinsic* node, const SwContext& context) {
    switch (node->intrinsic_type) {
        case Intrinsic::SIZEOF: {
            llvm::Type* val_type = codegen(&node->args.at(0), context).getRValue(*this, context)->getType();
            if (val_type->isPointerTy()) {
                return CGValue::rValue(toLLVMInt(getDataLayout().getPointerSize(0)));
            } if (val_type->isVoidTy()) {
                return CGValue::rValue(toLLVMInt(0));
            } return CGValue::rValue(toLLVMInt(getDataLayout().getTypeSizeInBits(val_type) / 8));
        }
        case Intrinsic::TYPEOF:
            return codegen(&node->args.at(0), context);
        case Intrinsic::ADV_PTR: {
            assert(node->args.size() == 2);
            assert(node->args.at(0).expr_type->isPointerType());
            assert(node->args.at(1).expr_type->isIntegral());

            llvm::Value* ptr = codegen(&node->args.at(0), context).getRValue(*this, context);
            std::array operands{codegen(&node->args.at(1), context).getRValue(*this, context)};

            return CGValue::rValue(Builder.CreateGEP(
                codegen(node->args.at(0).expr_type->getWrappedType(), context),
                ptr, operands
            ));
        }

        default:
            throw std::runtime_error("Intrinsic::llvmCodegen: Unknown intrinsic");
    }
}


CGValue LLVMBackend::llvmCodegen(ReturnStatement* node, const SwContext& context) {
    if (node->value.expr) {
        assert(node->parent_fn_type->ret_type != nullptr);
        auto new_ctx = context;
        new_ctx.bound_type = node->parent_fn_type->ret_type;
        llvm::Value* ret = codegen(&node->value, new_ctx).getRValue(*this, new_ctx);
        Builder.CreateRet(ret);
        return {};
    }

    Builder.CreateRetVoid();
    return {};
}


CGValue LLVMBackend::llvmCodegen(Struct* node, const SwContext& context) {
    const auto struct_sw_ty = SymMan.lookupType(node->ident);
    assert(struct_sw_ty);

    for (auto& member : node->members) {
        if (member->getNodeType() == ND_FUNC) {
            codegen(member, context);
        }
    }
    return {};
}


CGValue LLVMBackend::llvmCodegen(FuncCall* node, const SwContext& context) {
    std::vector<llvm::Type*> paramTypes;

    assert(node->ident.getIdentInfo());
    const auto fn_name = node->ident.getIdentInfo();

    llvm::Function* func = LModule->getFunction(mangleString(fn_name));
    if (!func) {
        [[maybe_unused]] auto fn = llvm::Function::Create(
            llvm::dyn_cast<llvm::FunctionType>(codegen(SymMan.lookupType(node->ident.getIdentInfo()), context)),
            llvm::GlobalValue::ExternalLinkage,
            mangleString(fn_name),
            LModule.get()
            );
        func = LModule->getFunction(mangleString(fn_name));
    }

    std::vector<llvm::Value*> arguments{};
    arguments.reserve(node->args.size() + 1);

    assert(node->ident.value);
    auto& entry = SymMan.lookupDecl(node->ident.value);
    
    if (entry.method_of && !entry.is_static) {
        // push the implicit instance pointer if the callee is a method and not static
        assert(ComputedPtr);
        arguments.push_back(ComputedPtr);  // push the ComputedPtr as an implicit argument
    }

    for (auto& item : node->args)
        arguments.push_back(codegen(&item, context).getRValue(*this, context));

    if (!func->getReturnType()->isVoidTy()) {
        auto call = Builder.CreateCall(func, arguments, node->ident.value->toString());
        return {call, call};
    }

    Builder.CreateCall(func, arguments);
    return {};
}


CGValue LLVMBackend::llvmCodegen(Var* node, const SwContext& context) {
    assert(node->var_type->type != nullptr);

    llvm::Type* type = codegen(node->var_type->type, context);
    assert(type != nullptr);

    llvm::Value* init = nullptr;

    auto linkage = (node->is_exported || node->is_extern) ?
        llvm::GlobalVariable::ExternalLinkage : llvm::GlobalVariable::InternalLinkage;


    if (!isLocalScope()) {
        auto var_name = node->is_extern ? node->var_ident->toString() : mangleString(node->var_ident);
        auto* var = new llvm::GlobalVariable(
                *LModule, type, node->is_const, linkage,
                nullptr, var_name
                );

        if (node->initialized) {
            auto new_ctx = context;
            new_ctx.bound_memory = var;
            init = codegen(&node->value, new_ctx).getRValue(*this, new_ctx);
            const auto val = llvm::dyn_cast<llvm::Constant>(init);
            assert(val != nullptr);
            var->setInitializer(val);
        } else if (node->value.expr == nullptr) {
            // when the `undefined` keyword isn't used, initialize with 0's
            var->setInitializer(llvm::Constant::getNullValue(type));
        }

        // handle references
        if (node->var_type->type->getTypeTag() == Type::REFERENCE) {
            SymMan.lookupDecl(node->var_ident).llvm_value = RefMemory;
        } else SymMan.lookupDecl(node->var_ident).llvm_value = var;
    } else {
        llvm::AllocaInst* var_alloca = Builder.CreateAlloca(
            type, nullptr, node->var_ident->toString());

        if (node->is_extern)
            return {};

        if (node->initialized) {
            // instance.BoundMemory = var_alloca;
            auto new_ctx = context;
            new_ctx.bound_type = node->var_type->type;
            init = codegen(&node->value, new_ctx).getRValue(*this, new_ctx);

            assert(init != nullptr);
            Builder.CreateStore(init, var_alloca, node->is_volatile);

        } else if (node->value.expr == nullptr) {
            // when the `undefined` keyword isn't used, initialize with 0's
            Builder.CreateMemSet(
                var_alloca,
                toLLVMInt(0, 8),
                getDataLayout().getTypeAllocSize(type),
                llvm::MaybeAlign());

            // TODO: make the memset inline in freestanding mode ^
        }

        // handle references
        if (node->var_type->type->getTypeTag() == Type::REFERENCE) {
            SymMan.lookupDecl(node->var_ident).llvm_value = RefMemory;
        } else SymMan.lookupDecl(node->var_ident).llvm_value = var_alloca;
    }

    return {};
}
