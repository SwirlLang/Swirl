#include <llvm/IR/DerivedTypes.h>
#include <types/SwTypes.h>
#include <backend/LLVMBackend.h>
#include <symbols/IdentManager.h>

#include "CompilerInst.h"


std::string FunctionType::toString() const {
    return ident->toString();
}

std::string StructType::toString() const {
    return ident->toString();
}

std::string PointerType::toString() const {
    std::string ret = (is_mutable ? "mut " : "") + of_type->toString();
    for (int i = 0; i < pointer_level; i++) {
        ret.append("*");
    } return std::move(ret);
}


llvm::Type* FunctionType::llvmCodegen(LLVMBackend& instance) {
    std::vector<llvm::Type*> llvm_param_types;
    llvm_param_types.reserve(this->param_types.size());

    for (const auto& ptr : this->param_types)
        llvm_param_types.push_back(ptr->llvmCodegen(instance));

    llvm::FunctionType* function = llvm::FunctionType::get(
        ret_type == nullptr ? llvm::Type::getVoidTy(instance.Context) : this->ret_type->llvmCodegen(instance),
        llvm_param_types,
        false
    );

    return function;
}

llvm::Type* TypeStr::llvmCodegen(LLVMBackend& instance) {
    if (instance.LLVMTypeCache.contains(this)) {
        return instance.LLVMTypeCache[this];
    }

    auto lit_arr = llvm::ArrayType::get(llvm::Type::getInt8Ty(instance.Context), size);

    const auto ret = llvm::StructType::create(instance.Context, "__Str");
    ret->setBody({lit_arr});

    instance.LLVMTypeCache[this] = ret;
    return ret;
}


llvm::Type* StructType::llvmCodegen(LLVMBackend& instance) {
    if (instance.LLVMTypeCache.contains(this)) {
        return instance.LLVMTypeCache[this];
    }

    std::vector<llvm::Type*> llvm_fields;
    llvm_fields.reserve(field_types.size());

    for (const auto& field : field_types) {
        llvm_fields.push_back(field->llvmCodegen(instance));
    }

    const auto struct_t = llvm::StructType::create(instance.Context, this->ident->toString());
    struct_t->setBody(llvm_fields);

    instance.LLVMTypeCache[this] = struct_t;
    return struct_t;
}


llvm::Type* PointerType::llvmCodegen(LLVMBackend& instance) {
    return llvm::PointerType::get(of_type->llvmCodegen(instance), 0);
}


llvm::Type* SliceType::llvmCodegen(LLVMBackend& instance) {
    if (instance.LLVMTypeCache.contains(this)) {
        return instance.LLVMTypeCache[this];
    }

     const auto struct_t = llvm::StructType::create(
        instance.Context,
        "__Slice"
    );

    struct_t->setBody({
        instance.SymMan.getPointerType(of_type, 1)
            ->llvmCodegen(instance),
        llvm::Type::getInt64Ty(instance.Context)
    });

    instance.LLVMTypeCache[this] = struct_t;
    return struct_t;
}


llvm::Type* VoidType::llvmCodegen(LLVMBackend& instance) {
    return llvm::Type::getVoidTy(instance.Context);
}

llvm::Type* ReferenceType::llvmCodegen(LLVMBackend& instance) {
    // references to arrays are compiled to their corresponding slice-types
    if (of_type->getSwType() == ARRAY)
        return instance.SymMan.getSliceType(of_type)->llvmCodegen(instance);
    return llvm::PointerType::get(of_type->llvmCodegen(instance), 0);
}

llvm::Type* ArrayType::llvmCodegen(LLVMBackend& instance) {
    if (instance.LLVMTypeCache.contains(this)) {
        return instance.LLVMTypeCache[this];
    }
    const auto arr_struct = llvm::StructType::create(instance.Context, "__Arr");
    arr_struct->setBody(llvm::ArrayType::get(of_type->llvmCodegen(instance), size));
    instance.LLVMTypeCache[this] = arr_struct;
    return arr_struct;
}


llvm::Type* TypeI8::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt8Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI32::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt32Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI16::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt16Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI64::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt64Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI128::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt128Ty(instance.Context);
    return ret;
}

llvm::Type* TypeF32::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getFloatTy(instance.Context);
    return ret;
}

llvm::Type* TypeF64::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getDoubleTy(instance.Context);
    return ret;
}

llvm::Type* TypeBool::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt1Ty(instance.Context);
    return ret;
}


// generate the method definitions for types which do not diverge significantly
#define DEFINE_CTYPE_DEF(Name, BitWidth, IsIntegral) \
    int  Name::getBitWidth() { return BitWidth; } \
    bool Name::isIntegral() { return IsIntegral; } \
    bool Name::isFloatingPoint() { return !IsIntegral; } \
    llvm::Type* Name::llvmCodegen(LLVMBackend& _) { \
        return llvm::Type::getIntNTy(_.Context, BitWidth); \
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


int TypeCL::getBitWidth() {
    const auto arch = llvm::Triple(CompilerInst::getTargetTriple()).getArch();
    if (arch == llvm::Triple::x86_64 || arch == llvm::Triple::aarch64)
        return 64;
    if (arch == llvm::Triple::x86 || arch == llvm::Triple::arm)
        return 32;
    throw std::runtime_error("TypeCL::isUnsigned: unsupported arch");
}

llvm::Type* TypeCL::llvmCodegen(LLVMBackend& instance) {
    return llvm::Type::getIntNTy(instance.Context, getBitWidth());
}

int TypeCSSizeT::getBitWidth() {
    return LLVMBackend::TargetMachine->getPointerSizeInBits(0);
}

llvm::Type* TypeCSSizeT::llvmCodegen(LLVMBackend& instance) {
    return llvm::Type::getIntNTy(instance.Context, getBitWidth());
}


int TypeCSizeT::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}

llvm::Type* TypeCSizeT::llvmCodegen(LLVMBackend& instance) {
    return TypeCSSizeT{}.llvmCodegen(instance);
}

int TypeCUL::getBitWidth() {
    return TypeCL{}.getBitWidth();
}

llvm::Type* TypeCUL::llvmCodegen(LLVMBackend& instance) {
    return TypeCL{}.llvmCodegen(instance);
}


int TypeCPtrDiffT::getBitWidth() {
    return LLVMBackend::TargetMachine->getPointerSizeInBits(0);
}

llvm::Type* TypeCPtrDiffT::llvmCodegen(LLVMBackend& instance) {
    return llvm::Type::getIntNTy(instance.Context, getBitWidth());
}


int TypeCWChar::getBitWidth() {
    const auto triple = llvm::Triple(CompilerInst::getTargetTriple());
    if (triple.getOS() == llvm::Triple::Win32) {
        return 16;
    } return 32;
}


llvm::Type* TypeCWChar::llvmCodegen(LLVMBackend& instance) {
    return llvm::Type::getIntNTy(instance.Context, getBitWidth());
}


int TypeCLDouble::getBitWidth() {
    const auto triple = llvm::Triple(CompilerInst::getTargetTriple());

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


llvm::Type* TypeCLDouble::llvmCodegen(LLVMBackend& instance) {
    const auto triple = llvm::Triple(CompilerInst::getTargetTriple());

    if (triple.getOS() == llvm::Triple::Linux && triple.getArch() == llvm::Triple::x86) {
        return llvm::Type::getX86_FP80Ty(instance.Context);
    }

    if ((triple.getOS() == llvm::Triple::Linux || triple.getOS() == llvm::Triple::Darwin) &&
        triple.getArch() == llvm::Triple::x86_64) {
        return llvm::Type::getFP128Ty(instance.Context);
        }

    return llvm::Type::getDoubleTy(instance.Context);
}


int TypeCIntPtr::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}


llvm::Type* TypeCIntPtr::llvmCodegen(LLVMBackend& instance) {
    return TypeCSSizeT{}.llvmCodegen(instance);
}


int TypeCUIntPtr::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}


llvm::Type* TypeCUIntPtr::llvmCodegen(LLVMBackend& instance) {
    return TypeCSSizeT{}.llvmCodegen(instance);
}

