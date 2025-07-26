#pragma once
#include <format>
#include <vector>

#include <llvm/IR/Type.h>


class IdentInfo;
class  LLVMBackend;
struct Var;

class Scope;
struct Type {
    enum SwTypes {
        FUNCTION,
        STRUCT,

        I8,
        I16,
        I32,
        I64,
        I128,

        U8,
        U16,
        U32,
        U64,
        U128,

        F32,
        F64,
        BOOL,
        STR,
        REFERENCE,
        POINTER,
        ARRAY,
        SLICE,
        VOID,

        C_INT, C_UINT,
        C_LL, C_ULL,
        C_L, C_UL,

        C_SIZE_T, C_SSIZE_T,

        C_CHAR, C_SCHAR, C_UCHAR,
        C_SHORT, C_USHORT,
        C_BOOL,
        C_FLOAT, C_DOUBLE, C_LDOUBLE,
        C_INTPTR, C_UINTPTR,
        C_PTRDIFF_T, C_INTMAX, C_UINTMAX,
        C_WCHAR,
    };

    bool   is_const = false;
    Scope* scope = nullptr;  // the pointer to the namespace (if applicable) defined within the type

    virtual SwTypes     getSwType() = 0;

    virtual llvm::Type* llvmCodegen(LLVMBackend&) = 0;
    
    virtual bool     isIntegral() { return false; }
    virtual bool     isFloatingPoint() { return false; }
    virtual bool     isUnsigned() { return false; }
    virtual int      getBitWidth() { return -1; }

    [[nodiscard]] virtual std::string toString() const = 0;
    [[nodiscard]] virtual IdentInfo* getIdent() const { return nullptr; }

    virtual bool operator==(Type* other) { return getSwType() == other->getSwType(); }

    virtual ~Type() = default;
};


struct FunctionType final : Type {
    IdentInfo*         ident;
    Type*              ret_type;
    std::vector<Type*> param_types;

    SwTypes getSwType() override { return FUNCTION; }
    [[nodiscard]] IdentInfo* getIdent() const override { return ident; }

    [[nodiscard]] std::string toString() const override;

    bool operator==(Type* other) override {
        return other->getSwType() == FUNCTION && (param_types == dynamic_cast<FunctionType*>(other)->param_types);
    }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};


struct StructType final : Type {
    IdentInfo* ident;

    std::vector<Type*> field_types;
    std::unordered_map<std::string, std::size_t> field_offsets;

    SwTypes getSwType() override { return STRUCT; }

    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] IdentInfo* getIdent() const override { return ident; }

    bool operator==(Type* other) override {
        return other->getSwType() == STRUCT && (field_types == dynamic_cast<StructType*>(other)->field_types);
    }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};


struct ArrayType final : Type {
    Type* of_type    = nullptr;
    std::size_t size = 0;

    ArrayType(Type* of_type, const std::size_t size): of_type(of_type), size(size) {}

    SwTypes getSwType() override { return ARRAY; }

    [[nodiscard]] std::string toString() const override {
        return std::format("[{} | {}]", of_type->toString(), size);
    }

    llvm::Type* llvmCodegen(LLVMBackend&) override;
};


struct TypeStr final : Type {
    std::size_t size;

    explicit TypeStr(const std::size_t size): size(size) {}

    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes    getSwType() override { return STR; }

    [[nodiscard]] std::string toString() const override { return "str"; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct ReferenceType final : Type {
    Type* of_type = nullptr;

    ReferenceType() = default;
    explicit ReferenceType(Type* t) : of_type(t) {}

    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    [[nodiscard]] std::string toString() const override { return "&" + of_type->toString(); }

    SwTypes    getSwType() override { return REFERENCE; }

    bool operator==(Type* other) override {
        return other->getSwType() == REFERENCE && (of_type == dynamic_cast<ReferenceType*>(other)->of_type);
    }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct PointerType final : Type {
    Type*    of_type = nullptr;
    uint16_t pointer_level{};

    PointerType() = default;
    explicit PointerType(Type* t, const uint16_t level): of_type(t), pointer_level(level) {}

    [[nodiscard]] std::string toString() const override;
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes    getSwType() override { return POINTER; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};


struct SliceType final : Type {
    Type* of_type;
    std::size_t size;

    explicit SliceType(Type* t, const std::size_t size) : of_type(t), size(size) {}

    [[nodiscard]] std::string toString() const override {
        return std::format("&[{} | {}]", of_type->toString(), size);
    }

    SwTypes getSwType() override { return SLICE; }
    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};


struct VoidType final : Type {
    [[nodiscard]] std::string toString() const override { return "void"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return VOID; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};


struct TypeI8 : Type {
    [[nodiscard]] std::string toString() const override { return "i8"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return I8; }

    bool isIntegral() override { return true; }
    int  getBitWidth() override { return 8; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct TypeI16 : Type {
    [[nodiscard]] std::string toString() const override { return "i16"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return I16; }

    bool isIntegral() override { return true; }
    int getBitWidth() override { return 16; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct TypeI32 : Type {
    [[nodiscard]] std::string toString() const override { return "i32"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return I32; }

    bool isIntegral() override { return true; }
    int getBitWidth() override { return 32; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct TypeI64 : Type {
    [[nodiscard]] std::string toString() const override { return "i64"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return I64; }

    bool isIntegral() override { return true; }
    int getBitWidth() override { return 64; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct TypeI128 : Type {
    [[nodiscard]] std::string toString() const override { return "i128"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return I128; }

    bool isIntegral() override { return true; }
    int getBitWidth() override { return 128; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

// Unsigned types
struct TypeU8 final : TypeI8 {
    [[nodiscard]] std::string toString() const override { return "u8"; }
    SwTypes getSwType() override { return U8; }
    bool isUnsigned() override { return true; }
};

struct TypeU16 final : TypeI16 {
    [[nodiscard]] std::string toString() const override { return "u16"; }
    SwTypes getSwType() override { return U16; }
    bool isUnsigned() override { return true; }
};

struct TypeU32 final : TypeI32 {
    [[nodiscard]] std::string toString() const override { return "u32"; }
    SwTypes getSwType() override { return U32; }
    bool isUnsigned() override { return true; }
};

struct TypeU64 final : TypeI64 {
    [[nodiscard]] std::string toString() const override { return "u64"; }
    SwTypes getSwType() override { return U64; }
    bool isUnsigned() override { return true; }
};

struct TypeU128 final : TypeI128 {
    [[nodiscard]] std::string toString() const override { return "u128"; }
    SwTypes getSwType() override { return U128; }
    bool isUnsigned() override { return true; }
};

struct TypeF32 final : Type {
    [[nodiscard]] std::string toString() const override { return "f32"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return F32; }

    bool isFloatingPoint() override { return true; }
    int getBitWidth() override { return 32; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct TypeF64 final : Type {
    [[nodiscard]] std::string toString() const override { return "f64"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return F64; }

    bool isFloatingPoint() override { return true; }
    int getBitWidth() override { return 64; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};

struct TypeBool final : Type {
    [[nodiscard]] std::string toString() const override { return "bool"; }
    [[nodiscard]] IdentInfo* getIdent() const override { return nullptr; }
    SwTypes getSwType() override { return BOOL; }

    int getBitWidth() override { return 1; }

    llvm::Type* llvmCodegen(LLVMBackend& instance) override;
};


// C types
#define DEFINE_CTYPE(Name, StringName, Tag) \
struct Name final : Type { \
    bool is_usn; \
    [[nodiscard]] std::string toString() const override { return StringName; } \
    SwTypes getSwType() override { return Tag; } \
    int getBitWidth() override; \
    bool isIntegral() override; \
    bool isFloatingPoint() override; \
    bool isUnsigned() override { return is_usn; } \
    explicit Name(bool is_unsigned = false): is_usn(is_unsigned)  {} \
    llvm::Type* llvmCodegen(LLVMBackend&) override; \
};

DEFINE_CTYPE(TypeCInt,       "c_int",       C_INT)
DEFINE_CTYPE(TypeCUInt,      "c_uint",      C_UINT)
DEFINE_CTYPE(TypeCLL,        "c_ll",        C_LL)
DEFINE_CTYPE(TypeCULL,       "c_ull",       C_ULL)
DEFINE_CTYPE(TypeCL,         "c_l",         C_L)
DEFINE_CTYPE(TypeCUL,        "c_ul",        C_UL)
DEFINE_CTYPE(TypeCSizeT,     "size_t",      C_SIZE_T)
DEFINE_CTYPE(TypeCSSizeT,    "c_ssize_t",   C_SSIZE_T)
DEFINE_CTYPE(TypeCSChar,     "c_schar",     C_SCHAR)
DEFINE_CTYPE(TypeCUChar,     "c_uchar",     C_UCHAR)
DEFINE_CTYPE(TypeCShort,     "c_short",     C_SHORT)
DEFINE_CTYPE(TypeCUShort,    "c_ushort",    C_USHORT)
DEFINE_CTYPE(TypeCBool,      "c_bool",      C_BOOL)
DEFINE_CTYPE(TypeCFloat,     "c_float",     C_FLOAT)
DEFINE_CTYPE(TypeCDouble,    "c_double",    C_DOUBLE)
DEFINE_CTYPE(TypeCLDouble,   "c_ldouble",   C_LDOUBLE)
DEFINE_CTYPE(TypeCIntPtr,    "c_intptr_t",  C_INTPTR)
DEFINE_CTYPE(TypeCUIntPtr,   "c_uintptr_t", C_UINTPTR)
DEFINE_CTYPE(TypeCPtrDiffT,  "c_ptrdiff_t", C_PTRDIFF_T)
DEFINE_CTYPE(TypeCIntMax,    "c_intmax_t",  C_INTMAX)
DEFINE_CTYPE(TypeCUIntMax,   "c_uintmax_t", C_UINTMAX)
DEFINE_CTYPE(TypeCWChar,     "c_wchar_t",   C_WCHAR)

#undef DEFINE_CTYPE

