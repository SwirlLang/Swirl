#include "types/SwTypes.h"
#include "CompilerInst.h"
#include "types/definitions.h"
#include "symbols/IdentManager.h"
#include "../../include/backend/LLVMBackend.h"


#include <llvm/IR/DerivedTypes.h>
#include <llvm/TargetParser/Triple.h>



std::string FunctionType::toString() const {
    return ident->toString();
}

std::string StructType::toString() const {
    return ident->toString();
}

std::string PointerType::toString() const {
    return is_mutable
        ? "mut " + of_type->toString() + '*'
        : "" + of_type->toString() + '*';
}

std::string EnumType::toString() const {
    return "enum " + id->toString();
}


llvm::Type* LLVMBackend::llvmCodegen(FunctionType* type, const SwContext& ctx) {
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

llvm::Type* LLVMBackend::llvmCodegen(ReferenceType* type, const SwContext& context) {
    // references to strings compile to a slice (i8* + i64)
    if (type->of_type->getTypeTag() == Type::STR) {
        SliceType ty{&GlobalTypeI8};
        return codegen(&ty, context);
    }

    return llvm::PointerType::get(LLVMContext, 0);

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


// generate the method definitions for types which do not diverge significantly
#define DEFINE_CTYPE_DEF(Name, BitWidth, IsIntegral) \
    unsigned int  Name::getBitWidth() { return BitWidth; } \
    bool Name::isIntegral() { return IsIntegral; } \
    bool Name::isFloatingPoint() { return !(IsIntegral); } \
    llvm::Type* LLVMBackend::llvmCodegen(Name* type, SwContext context) { \
        return llvm::Type::getIntNTy(LLVMContext, BitWidth); \
        }

#define DEFINE_ATTRIBUTES(Name, IsIntegral) \
    bool Name::isIntegral() { return IsIntegral; } \
    bool Name::isFloatingPoint() { return !IsIntegral; }

DEFINE_CTYPE_DEF(TypeCInt,       32, true )
DEFINE_CTYPE_DEF(TypeCUInt,      32, true )
DEFINE_CTYPE_DEF(TypeCLL,        64, true )
DEFINE_CTYPE_DEF(TypeCULL,       64, true )
DEFINE_CTYPE_DEF(TypeCSChar,      8, true )     // signed char
DEFINE_CTYPE_DEF(TypeCUChar,      8, true )      // unsigned char
DEFINE_CTYPE_DEF(TypeCShort,     16, true )
DEFINE_CTYPE_DEF(TypeCUShort,    16, true )
DEFINE_CTYPE_DEF(TypeCBool,       8, true )      // usually 1 byte and unsigned
DEFINE_CTYPE_DEF(TypeCFloat,     32, false)
DEFINE_CTYPE_DEF(TypeCDouble,    64, false)
DEFINE_CTYPE_DEF(TypeCIntMax,    64, true )
DEFINE_CTYPE_DEF(TypeCUIntMax,   64, true )

DEFINE_ATTRIBUTES(TypeCL, true)
DEFINE_ATTRIBUTES(TypeCUL, true)
DEFINE_ATTRIBUTES(TypeCSizeT, true)
DEFINE_ATTRIBUTES(TypeCSSizeT, true)
DEFINE_ATTRIBUTES(TypeCPtrDiffT, true)

DEFINE_ATTRIBUTES(TypeCWChar, true)
DEFINE_ATTRIBUTES(TypeCLDouble, false)
DEFINE_ATTRIBUTES(TypeCIntPtr, true)
DEFINE_ATTRIBUTES(TypeCUIntPtr, true)

#undef DEFINE_CTYPE_DEF
#undef DEFINE_ATTRIBUTES


unsigned int fetchPointerSize(const llvm::Triple::ArchType arch) {
    switch (arch) {
        case llvm::Triple::x86:
        case llvm::Triple::arm:
            return 4;
        case llvm::Triple::x86_64:
        case llvm::Triple::aarch64:
            return 8;
        default:
            throw std::runtime_error("fetchPointerSize: Unsupported architecture");
    }
}

unsigned int TypeCL::getBitWidth() {
    const auto arch = llvm::Triple(CompilerInst::TargetTriple).getArch();
    if (arch == llvm::Triple::x86_64 || arch == llvm::Triple::aarch64)
        return 64;
    if (arch == llvm::Triple::x86 || arch == llvm::Triple::arm)
        return 32;
    throw std::runtime_error("TypeCL::isUnsigned: unsupported arch");
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCL* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}

unsigned int TypeCSSizeT::getBitWidth() {
    return fetchPointerSize(llvm::Triple(CompilerInst::TargetTriple).getArch());
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCSSizeT* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}


unsigned int TypeCSizeT::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCSizeT* type, const SwContext& ctx) {
    auto a = TypeCSSizeT{};
    return codegen(&a, ctx);
}

unsigned int TypeCUL::getBitWidth() {
    return TypeCL{}.getBitWidth();
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCUL* type, SwContext context) {
    auto a = TypeCL{};
    return codegen(&a, context);
}


unsigned int TypeCPtrDiffT::getBitWidth() {
    return fetchPointerSize(llvm::Triple(CompilerInst::TargetTriple).getArch());
}

llvm::Type* LLVMBackend::llvmCodegen(TypeCPtrDiffT* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}


unsigned int TypeCWChar::getBitWidth() {
    const auto triple = llvm::Triple(CompilerInst::TargetTriple);
    if (triple.getOS() == llvm::Triple::Win32) {
        return 16;
    } return 32;
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCWChar* type, SwContext) {
    return llvm::Type::getIntNTy(LLVMContext, type->getBitWidth());
}


unsigned int TypeCLDouble::getBitWidth() {
    const auto triple = llvm::Triple(CompilerInst::TargetTriple);

    if (triple.getArch() == llvm::Triple::x86_64) {
        if (triple.getOS() == llvm::Triple::Win32) {
            return 64;
        }
        if (triple.getOS() == llvm::Triple::Linux || triple.getOS() == llvm::Triple::Darwin) {
            return 128; // 80-bit x87 extended precision, padded to 128 bits
        }
    }

    if (triple.getArch() == llvm::Triple::x86) {
        if (triple.getOS() == llvm::Triple::Win32) {
            return 64;
        }
        if (triple.getOS() == llvm::Triple::Linux) {
            return 96; // 80-bit x87 extended, padded to 96
        }
    }

    if (triple.getArch() == llvm::Triple::aarch64) {
        // 128 with -mlong-double-128 not handled
        return 64;
    }

    if (triple.getArch() == llvm::Triple::arm) {
        return 64;
    }

    throw std::runtime_error("TypeCSSizeT::getBitWidth: unsupported arch");
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCLDouble* type, SwContext) {
    const auto triple = llvm::Triple(CompilerInst::TargetTriple);

    if (triple.getOS() == llvm::Triple::Linux && triple.getArch() == llvm::Triple::x86) {
        return llvm::Type::getX86_FP80Ty(LLVMContext);
    }

    if ((triple.getOS() == llvm::Triple::Linux || triple.getOS() == llvm::Triple::Darwin) &&
        triple.getArch() == llvm::Triple::x86_64) {
        return llvm::Type::getFP128Ty(LLVMContext);
        }

    return llvm::Type::getDoubleTy(LLVMContext);
}


unsigned int TypeCIntPtr::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCIntPtr*, SwContext ctx) {
    auto a = TypeCSSizeT{};
    return codegen(&a, ctx);
}


unsigned int TypeCUIntPtr::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}


llvm::Type* LLVMBackend::llvmCodegen(TypeCUIntPtr* type, const SwContext& context) {
    auto a = TypeCSSizeT{};
    return codegen(&a, context);
}

