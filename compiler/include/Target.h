#pragma once
#include <algorithm>
#include <string>
#include <stdexcept>

#include "types/SwTypes.h"


namespace sw {
class Target {
public:
    enum class Architecture {
        x64,
        x86,
        ARM64,
        ARM32,
        Unknown
    };

    enum class Platform {
        Linux,
        Windows,
        Darwin,
        Unknown
    };

    enum class Environment {
        GNU,
        Musl,
        MSVC,
        Unknown
    };

    // convenience aliases
    static constexpr Architecture x64   = Architecture::x64;
    static constexpr Architecture x86   = Architecture::x86;
    static constexpr Architecture ARM64 = Architecture::ARM64;
    static constexpr Architecture ARM32 = Architecture::ARM32;

    static constexpr Platform Linux     = Platform::Linux;
    static constexpr Platform Windows   = Platform::Windows;
    static constexpr Platform Darwin    = Platform::Darwin;

    static constexpr Environment GNU   = Environment::GNU;
    static constexpr Environment Musl  = Environment::Musl;
    static constexpr Environment MSVC  = Environment::MSVC;


    class Triple_t {
    public:
        [[nodiscard]]
        Architecture getArch() const {
            return m_Arch;
        }

        [[nodiscard]]
        Platform getOS() const {
            return m_Platform;
        }

        [[nodiscard]]
        Environment getEnvironment() const {
            return m_Environment;
        }

        [[nodiscard]]
        std::string toString() const {
            std::string result;

            // arch
            switch (m_Arch) {
                case Architecture::x64:   result = "x86_64";      break;
                case Architecture::x86:   result = "i686";        break;
                case Architecture::ARM64: result = "aarch64";     break;
                case Architecture::ARM32: result = "arm";         break;
                default:                  result = "unknown";     break;
            }

            // vendor
            result += '-';
            result += (m_Platform == Platform::Darwin) ? "apple" : "unknown";

            // os
            result += '-';
            switch (m_Platform) {
                case Platform::Linux:   result += "linux";    break;
                case Platform::Windows: result += "windows";  break;
                case Platform::Darwin:  result += "darwin";   break;
                default:                result += "unknown";  break;
            }

            // env
            result += '-';
            switch (m_Environment) {
                case Environment::GNU:   result += "gnu";   break;
                case Environment::Musl:  result += "musl";  break;
                case Environment::MSVC:  result += "msvc";  break;
                default:                 result += "unknown"; break;
            }

            return result;
        }

    private:
        friend class Target;

        Architecture m_Arch        {Architecture::Unknown};
        Platform     m_Platform    {Platform::Unknown};
        Environment  m_Environment {Environment::Unknown};
    };


    /// Returns a Target object for the host machine's Triple
    static Target fromHostTriple() {
        Target target;
        target.m_Initialized = true;

    #if defined(__x86_64__) || defined(_M_X64)
        target.m_Triple.m_Arch = x64;
    #elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
        target.m_Triple.m_Arch = x86;
    #elif defined(__aarch64__) || defined(_M_ARM64)
        target.m_Triple.m_Arch = ARM64;
    #elif defined(__arm__) || defined(_M_ARM)
        target.m_Triple.m_Arch = ARM32;
    #endif

    #if defined(_WIN32) || defined(_WIN64)
        target.m_Triple.m_Platform = Windows;
    #elif defined(__APPLE__) || defined(__MACH__)
        target.m_Triple.m_Platform = Darwin;
    #elif defined(__linux__)
        target.m_Triple.m_Platform = Linux;
    #endif

    // -------- environment --------

    #if defined(_WIN32) || defined(_WIN64)
        target.m_Triple.m_Environment = MSVC;
    #elif defined(__linux__)
        target.m_Triple.m_Environment = Musl;
    #endif

        return target;
    }


    /// Returns a Target object for the given triple
    static Target fromTriple(std::string_view triple) {
        Target target;
        target.m_Initialized = true;

        while (!triple.empty()) {
            const auto pos = triple.find('-');

            std::string_view token;

            if (pos == std::string_view::npos) {
                token = triple;
                triple = {};
            } else {
                token = triple.substr(0, pos);
                triple.remove_prefix(pos + 1);
            }

            // -------- arch --------

            if (token == "x86_64" || token == "amd64") {
                target.m_Triple.m_Arch = x64;
                continue;
            }

            if (token == "i386" ||
                token == "i486" ||
                token == "i586" ||
                token == "i686" ||
                token == "x86") {
                target.m_Triple.m_Arch = x86;
                continue;
                }

            if (token == "aarch64" || token == "arm64") {
                target.m_Triple.m_Arch = ARM64;
                continue;
            }

            if (token == "arm") {
                target.m_Triple.m_Arch = ARM32;
                continue;
            }

            // -------- os --------

            if (token == "linux") {
                target.m_Triple.m_Platform = Linux;
                continue;
            }

            if (token == "windows" ||
                token == "win32"   ||
                token == "mingw32" ||
                token == "cygwin") {
                target.m_Triple.m_Platform = Windows;
                continue;
                }

            if (token == "darwin"  ||
                token == "macos"   ||
                token == "macosx") {
                target.m_Triple.m_Platform = Darwin;
                continue;
                }

            // -------- env --------

            if (token == "gnu"   || token == "gnueabi" ||
                token == "gnueabihf") {
                target.m_Triple.m_Environment = GNU;
                continue;
            }

            if (token == "musl") {
                target.m_Triple.m_Environment = Musl;
                continue;
            }

            if (token == "msvc") {
                target.m_Triple.m_Environment = MSVC;
            }
        }

        return target;
    }


    [[nodiscard]]
    const Triple_t& getTriple() const { return m_Triple; }

    [[nodiscard]]
    bool isInitialized() const { return m_Initialized; }

    [[nodiscard]]
    std::string toString() const { return m_Triple.toString(); }


    std::size_t getAlignment(Type* type) const {
        switch (type->kind) {

            // ---- swirl types ----

            case Type::BOOL:
            case Type::CHAR:
            case Type::I8:
            case Type::U8:     return 1;
            case Type::I16:
            case Type::U16:    return 2;
            case Type::I32:
            case Type::U32:    return 4;
            case Type::I64:
            case Type::U64:    return 8;
            case Type::I128:
            case Type::U128:   return 16;
            case Type::F32:    return 4;
            case Type::F64:    return 8;

            // ---- composite types ----

            case Type::POINTER:
            case Type::REFERENCE:
                return getPointerSize();

            case Type::STR:
            case Type::SLICE:
                return 8;  // { ptr, i64 } — 8-byte aligned due to i64

            case Type::ARRAY:
                return getAlignment(type->to<ArrayType>()->of_type);
            case Type::ENUM:
                return getAlignment(type->to<EnumType>()->of_type);
            case Type::STRUCT: {
                std::size_t maxAlign = 1;
                for (auto* field : type->to<StructType>()->field_types)
                    maxAlign = std::max(maxAlign, getAlignment(field));
                return maxAlign;
            }

            // ---- C types ----

            case Type::C_CHAR:
            case Type::C_SCHAR:
            case Type::C_UCHAR:
            case Type::C_BOOL:
                return 1;

            case Type::C_SHORT:
            case Type::C_USHORT:
                return 2;

            case Type::C_INT:
            case Type::C_UINT:
            case Type::C_FLOAT:
                return 4;

            case Type::C_DOUBLE:
            case Type::C_LL:
            case Type::C_ULL:
            case Type::C_INTMAX:
            case Type::C_UINTMAX:
                return 8;

            case Type::C_L:
            case Type::C_UL:
            case Type::C_SIZE_T:
            case Type::C_SSIZE_T:
            case Type::C_INTPTR:
            case Type::C_UINTPTR:
            case Type::C_PTRDIFF_T:
                return getPointerSize();

            case Type::C_WCHAR:
                return m_Triple.getOS() == Platform::Windows ? 2 : 4;

            case Type::C_LDOUBLE:
                return getLDoubleAlignment();

                // ---- unresolvable ----

            case Type::VOID:
            case Type::FUNCTION:
            case Type::GENERIC:
            case Type::UNI_TY:
            default:
                return 1;
        }
    }


    std::size_t getSizeInBits(Type* type) const {
        switch (type->kind) {

            // ---- swirl types ----

            case Type::BOOL:   return 1;
            case Type::CHAR:
            case Type::I8:
            case Type::U8:     return 8;
            case Type::I16:
            case Type::U16:    return 16;
            case Type::I32:
            case Type::U32:    return 32;
            case Type::I64:
            case Type::U64:    return 64;
            case Type::I128:
            case Type::U128:   return 128;
            case Type::F32:    return 32;
            case Type::F64:    return 64;

            // ---- composite types ----

            case Type::POINTER:
            case Type::REFERENCE:
                return getPointerSize() * 8;

            case Type::STR:
            case Type::SLICE: {
                // struct { ptr, i64 }
                std::size_t ptrBits = getPointerSize() * 8;
                return ((ptrBits + 63) & ~std::size_t{63}) + 64;
            }

            case Type::ARRAY: {
                auto* arr = type->to<ArrayType>();
                return getSizeInBits(arr->of_type) * arr->size;
            }

            case Type::ENUM:
                return getSizeInBits(type->to<EnumType>()->of_type);

            case Type::STRUCT:
                return computeStructSizeBits(type->to<StructType>());

            // ---- C types  ----

            case Type::C_CHAR:
            case Type::C_SCHAR:
            case Type::C_UCHAR:
            case Type::C_BOOL:
                return 8;

            case Type::C_SHORT:
            case Type::C_USHORT:
                return 16;

            case Type::C_INT:
            case Type::C_UINT:
            case Type::C_FLOAT:
                return 32;

            case Type::C_DOUBLE:
            case Type::C_LL:
            case Type::C_ULL:
            case Type::C_INTMAX:
            case Type::C_UINTMAX:
                return 64;

            case Type::C_L:
            case Type::C_UL:
            case Type::C_SIZE_T:
            case Type::C_SSIZE_T:
            case Type::C_INTPTR:
            case Type::C_UINTPTR:
            case Type::C_PTRDIFF_T:
                return getPointerSize() * 8;

            case Type::C_WCHAR:
                return (m_Triple.getOS() == Platform::Windows) ? 16 : 32;

            case Type::C_LDOUBLE:
                return getLDoubleBitWidth();

            case Type::VOID:
            case Type::FUNCTION:
            case Type::GENERIC:
            case Type::UNI_TY:
            default:
                throw std::runtime_error("getSizeInBits: type has no specified size");
        }
    }


    [[nodiscard]]  /// Returns pointer size for the target in bytes
    std::size_t getPointerSize() const {
        switch (m_Triple.getArch()) {
            case Architecture::x86:
            case Architecture::ARM32:
                return 4;
            default:
                return 8;
        }
    }


private:
    Triple_t m_Triple;
    bool     m_Initialized = false;


    [[nodiscard]]
    std::size_t getLDoubleAlignment() const {
        const auto arch = m_Triple.getArch();
        const auto os   = m_Triple.getOS();

        if (arch == Architecture::x64) {
            if (os == Platform::Linux || os == Platform::Darwin)
                return 16;
            return 8;
        }

        if (arch == Architecture::x86) {
            if (os == Platform::Linux)
                return 4;
            return 8;
        }

        return 8;
    }


    [[nodiscard]]
    std::size_t getLDoubleBitWidth() const {
        const auto arch = m_Triple.getArch();
        const auto os   = m_Triple.getOS();

        if (arch == Architecture::x64) {
            if (os == Platform::Linux || os == Platform::Darwin)
                return 128;
            return 64;
        }

        if (arch == Architecture::x86) {
            if (os == Platform::Linux)
                return 96;
            return 64;
        }

        return 64;
    }


    std::size_t computeStructSizeBits(StructType* st) const {
        std::size_t sizeBits = 0;

        for (auto* field : st->field_types) {
            const std::size_t fieldAlignBits = getAlignment(field) * 8;
            sizeBits = (sizeBits + fieldAlignBits - 1) & ~(fieldAlignBits - 1);
            sizeBits += getSizeInBits(field);
        }

        const std::size_t structAlignBits = getAlignment(st) * 8;
        return (sizeBits + structAlignBits - 1) & ~(structAlignBits - 1);
    }
};
}
