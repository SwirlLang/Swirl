#include "CompilerInst.h"
#include "Target.h"
#include "backend/LLVMBackend.h"


llvm::Type* LLVMBackend::llvmCodegen(const FunctionType* type, const SwContext& ctx) {
    std::vector<llvm::Type*> llvm_param_types;
    llvm_param_types.reserve(type->param_types.size());

    for (const auto& ptr : type->param_types)
        llvm_param_types.push_back(codegen(ptr, ctx));

    llvm::FunctionType* function = llvm::FunctionType::get(
        type->ret_type == nullptr ? llvm::Type::getVoidTy(LLVMContext) : codegen(type->ret_type, ctx),
        llvm_param_types,
        false
    );

    return function;
}


llvm::Type* LLVMBackend::llvmCodegen(EnumType* type, const SwContext& ctx) {
    return codegen(type->of_type, ctx);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeStr* type, SwContext ctx) {
    if (LLVMTypeCache.contains(type)) {
        return LLVMTypeCache[type];
    }

    const auto ret = codegen(SymMan.lookupType(SymMan.getIdInfoOfAGlobal("str")), ctx);
    LLVMTypeCache[type] = ret;
    return ret;
}


llvm::Type* LLVMBackend::llvmCodegen(StructType* type, const SwContext& ctx) {
    if (LLVMTypeCache.contains(type)) {
        return LLVMTypeCache[type];
    }

    std::vector<llvm::Type*> llvm_fields;
    llvm_fields.reserve(type->field_types.size());

    for (const auto& field : type->field_types) {
        llvm_fields.push_back(codegen(field, ctx));
    }

    if (llvm_fields.empty()) {
        llvm_fields.push_back(llvm::Type::getInt8Ty(LLVMContext));
    }

    const auto struct_t = llvm::StructType::create(LLVMContext, type->ident->toString());
    struct_t->setBody(llvm_fields);

    LLVMTypeCache[type] = struct_t;
    return struct_t;
}


llvm::Type* LLVMBackend::llvmCodegen(PointerType*, SwContext) {
    return llvm::PointerType::get(LLVMContext, 0);
}


llvm::Type* LLVMBackend::llvmCodegen(SliceType* type, const SwContext& context) {
    if (LLVMTypeCache.contains(type)) {
        return LLVMTypeCache[type];
    }

    const auto struct_t = llvm::StructType::create(LLVMContext, "__Slice");

    struct_t->setBody({
        codegen(SymMan.getPointerType(type->of_type, false), context),  // pointer to the first element
        llvm::Type::getInt64Ty(LLVMContext)  // size
    });

    LLVMTypeCache[type] = struct_t;
    return struct_t;
}


llvm::Type* LLVMBackend::llvmCodegen(VoidType*, SwContext) {
    return llvm::Type::getVoidTy(LLVMContext);
}


llvm::Type* LLVMBackend::llvmCodegen(const GenericType* type, const SwContext& context) {
    if (type->contained_type) {
        return codegen(type->contained_type, context);
    } throw std::runtime_error(
        std::format("GenericType::llvmCodegen: no contained type! id={}",
            type->id ? type->id->toString() : "nullptr"));
}


llvm::Type* LLVMBackend::llvmCodegen(ReferenceType* type, const SwContext& context) {
    // references to strings compile to a slice (i8* + i64)
    if (type->of_type->getTypeTag() == Type::STR) {
        SliceType ty{&GlobalTypeI8};
        return codegen(&ty, context);
    } return llvm::PointerType::get(LLVMContext, 0);
}


llvm::Type* LLVMBackend::llvmCodegen(ArrayType* type, const SwContext& context) {
    if (LLVMTypeCache.contains(type)) {
        return LLVMTypeCache[type];
    }

    const auto arr_struct = llvm::StructType::create(LLVMContext, "__Arr");
    arr_struct->setBody(llvm::ArrayType::get(codegen(type->of_type, context), type->size));
    LLVMTypeCache[type] = arr_struct;
    return arr_struct;
}


llvm::Type* LLVMBackend::llvmCodegen(TypeChar*, SwContext) {
    return llvm::Type::getInt8Ty(LLVMContext);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeI32* type, SwContext) {
    return llvm::Type::getInt32Ty(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeI16* type, SwContext) {
    return llvm::Type::getInt16Ty(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeI8*, SwContext) {
    const auto ret = llvm::Type::getInt8Ty(LLVMContext);
    return ret;
}

llvm::Type* LLVMBackend::llvmCodegen(TypeI64* type, SwContext) {
    return llvm::Type::getInt64Ty(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeI128* type, SwContext) {
    return llvm::Type::getInt128Ty(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeF32* type, SwContext) {
    return llvm::Type::getFloatTy(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeF64* type, SwContext) {
    return llvm::Type::getDoubleTy(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeBool* type, SwContext) {
    return llvm::Type::getInt1Ty(LLVMContext);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeU8* type, SwContext context) {
    return llvm::Type::getInt8Ty(LLVMContext);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeU16* type, SwContext context) {
    return llvm::Type::getInt16Ty(LLVMContext);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeU32* type, SwContext context) {
    return llvm::Type::getInt32Ty(LLVMContext);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeU64* type, SwContext context) {
    return llvm::Type::getInt64Ty(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeU128* type, SwContext context) {
    return llvm::Type::getInt128Ty(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCL* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCUL* type, const SwContext& context) {
    auto a = TypeCL{};
    return codegen(&a, context);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCSSizeT* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCSizeT* type, const SwContext& ctx) {
    auto a = TypeCSSizeT{};
    return codegen(&a, ctx);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCPtrDiffT* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCWChar* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCLDouble* type, SwContext) {
    const auto triple = CompilerInst::Target.getTriple();

    if (triple.getOS() == sw::Target::Linux && triple.getArch() == sw::Target::x86) {
        return llvm::Type::getX86_FP80Ty(LLVMContext);
    }

    if ((triple.getOS() == sw::Target::Linux || triple.getOS() == sw::Target::Darwin) &&
        triple.getArch() == sw::Target::x64) {
        return llvm::Type::getFP128Ty(LLVMContext);
        }

    return llvm::Type::getDoubleTy(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCIntPtr*, SwContext ctx) {
    auto a = TypeCSSizeT{};
    return codegen(&a, ctx);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCUIntPtr* type, const SwContext& context) {
    auto a = TypeCSSizeT{};
    return codegen(&a, context);
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCInt* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 32);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCUInt* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 32);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCLL* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 64);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCULL* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 64);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCSChar* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 8);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCUChar* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 8);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCShort* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 16);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCUShort* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 16);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCBool* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 8);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCFloat* type, SwContext context) {
    return llvm::Type::getFloatTy(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCDouble* type, SwContext context) {
    return llvm::Type::getDoubleTy(LLVMContext);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCIntMax* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 64);
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCUIntMax* type, SwContext context) {
    return llvm::Type::getIntNTy(LLVMContext, 64);
}