#include "types/SwTypes.h"
#include "CompilerInst.h"
#include "symbols/IdentManager.h"


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


// generate the method definitions for types which do not diverge significantly
#define DEFINE_CTYPE_DEF(Name, BitWidth, IsIntegral) \
    unsigned int  Name::getBitWidth() { return BitWidth; } \
    bool Name::isIntegral() { return IsIntegral; } \
    bool Name::isFloatingPoint() { return !(IsIntegral); }

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


unsigned int fetchPointerSize(const sw::Target::Architecture arch) {
    switch (arch) {
        case sw::Target::x86:
        case sw::Target::ARM32:
            return 4;
        case sw::Target::x64:
        case sw::Target::ARM64:
            return 8;
        default:
            throw std::runtime_error("fetchPointerSize: Unsupported architecture");
    }
}

unsigned int TypeCL::getBitWidth() {
    const auto arch = CompilerInst::Target.getTriple().getArch();
    if (arch == sw::Target::x64 || arch == sw::Target::ARM64)
        return 64;
    if (arch == sw::Target::x86 || arch == sw::Target::ARM32)
        return 32;
    throw std::runtime_error("TypeCL::isUnsigned: unsupported arch");
}

unsigned int TypeCSSizeT::getBitWidth() {
    return fetchPointerSize(CompilerInst::Target.getTriple().getArch()) * 8;
}


unsigned int TypeCSizeT::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}

unsigned int TypeCUL::getBitWidth() {
    return TypeCL{}.getBitWidth();
}


unsigned int TypeCPtrDiffT::getBitWidth() {
    return fetchPointerSize(CompilerInst::Target.getTriple().getArch()) * 8;
}

unsigned int TypeCWChar::getBitWidth() {
    const auto triple = CompilerInst::Target.getTriple();
    if (triple.getOS() == sw::Target::Windows) {
        return 16;
    } return 32;
}


unsigned int TypeCLDouble::getBitWidth() {
    const auto triple = CompilerInst::Target.getTriple();

    if (triple.getArch() == sw::Target::x64) {
        if (triple.getOS() == sw::Target::Windows) {
            return 64;
        }
        if (triple.getOS() == sw::Target::Linux || triple.getOS() == sw::Target::Darwin) {
            return 128; // 80-bit x87 extended precision, padded to 128 bits
        }
    }

    if (triple.getArch() == sw::Target::x86) {
        if (triple.getOS() == sw::Target::Windows) {
            return 64;
        }
        if (triple.getOS() == sw::Target::Linux) {
            return 96; // 80-bit x87 extended, padded to 96
        }
    }

    if (triple.getArch() == sw::Target::ARM64) {
        // 128 with -mlong-double-128 not handled
        return 64;
    }

    if (triple.getArch() == sw::Target::ARM32) {
        return 64;
    }

    throw std::runtime_error("TypeCSSizeT::getBitWidth: unsupported arch");
}


unsigned int TypeCIntPtr::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}

unsigned int TypeCUIntPtr::getBitWidth() {
    return TypeCSSizeT{}.getBitWidth();
}
