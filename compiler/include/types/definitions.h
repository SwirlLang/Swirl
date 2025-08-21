#pragma once
#include <string_view>
#include <unordered_map>

#include "SwTypes.h"


inline TypeI8    GlobalTypeI8{};
inline TypeU8    GlobalTypeU8{};

inline TypeI16   GlobalTypeI16{};
inline TypeU16   GlobalTypeU16{};

inline TypeI32   GlobalTypeI32{};
inline TypeU32   GlobalTypeU32{};

inline TypeI64   GlobalTypeI64{};
inline TypeU64   GlobalTypeU64{};

inline TypeI128  GlobalTypeI128{};
inline TypeU128  GlobalTypeU128{};

inline TypeF32   GlobalTypeF32{};
inline TypeF64   GlobalTypeF64{};

inline TypeBool  GlobalTypeBool{};
inline TypeChar  GlobalTypeChar{};

inline VoidType  GlobalTypeVoid{};


// C Types
inline TypeCInt       GlobalTypeCInt{false};
inline TypeCUInt      GlobalTypeCUInt{true};

inline TypeCLL        GlobalTypeCLL{false};
inline TypeCULL       GlobalTypeCULL{true};
inline TypeCL         GlobalTypeCL{false};
inline TypeCUL        GlobalTypeCUL{true};
inline TypeCSizeT     GlobalTypeCSizeT{true};
inline TypeCSSizeT    GlobalTypeCSSizeT{false};
inline TypeCSChar     GlobalTypeCSChar{false};
inline TypeCUChar     GlobalTypeCUChar{true};
inline TypeCShort     GlobalTypeCShort{false};
inline TypeCUShort    GlobalTypeCUShort{true};
inline TypeCBool      GlobalTypeCBool{false};
inline TypeCFloat     GlobalTypeCFloat{false};
inline TypeCDouble    GlobalTypeCDouble{false};
inline TypeCLDouble   GlobalTypeCLDouble{false};
inline TypeCIntPtr    GlobalTypeCIntPtr{false};
inline TypeCUIntPtr   GlobalTypeCUIntPtr{true};
inline TypeCPtrDiffT  GlobalTypeCPtrDiffT{false};
inline TypeCIntMax    GlobalTypeCIntMax{false};
inline TypeCUIntMax   GlobalTypeCUIntMax{true};
inline TypeCWChar     GlobalTypeCWChar{false};  // Note: wchar_t signedness is implementation-defined

inline const
std::unordered_map<std::string_view, Type*> BuiltinTypes = {
    {"i8", &GlobalTypeI8},
    {"u8", &GlobalTypeU8},

    {"i16", &GlobalTypeI16},
    {"u16", &GlobalTypeU16},

    {"i32", &GlobalTypeI32},
    {"u32", &GlobalTypeU32},

    {"i64", &GlobalTypeI64},
    {"u64", &GlobalTypeU64},

    {"i128", &GlobalTypeI128},
    {"u128", &GlobalTypeU128},

    {"f32", &GlobalTypeF32},
    {"f64", &GlobalTypeF64},

    {"bool", &GlobalTypeBool},
    {"char", &GlobalTypeChar},

    {"void", &GlobalTypeVoid},


    // C Types
    {"c_int",       &GlobalTypeCInt},
    {"c_uint",      &GlobalTypeCUInt},
    {"c_ll",        &GlobalTypeCLL},
    {"c_ull",       &GlobalTypeCULL},
    {"c_l",         &GlobalTypeCL},
    {"c_ul",        &GlobalTypeCUL},
    {"c_size_t",    &GlobalTypeCSizeT},
    {"c_ssize_t",   &GlobalTypeCSSizeT},
    {"c_schar",     &GlobalTypeCSChar},
    {"c_uchar",     &GlobalTypeCUChar},
    {"c_short",     &GlobalTypeCShort},
    {"c_ushort",    &GlobalTypeCUShort},
    {"c_bool",      &GlobalTypeCBool},
    {"c_float",     &GlobalTypeCFloat},
    {"c_double",    &GlobalTypeCDouble},
    {"c_ldouble",   &GlobalTypeCLDouble},
    {"c_intptr_t",  &GlobalTypeCIntPtr},
    {"c_uintptr_t", &GlobalTypeCUIntPtr},
    {"c_ptrdiff_t", &GlobalTypeCPtrDiffT},
    {"c_intmax_t",  &GlobalTypeCIntMax},
    {"c_uintmax_t", &GlobalTypeCUIntMax},
    {"c_wchar_t",   &GlobalTypeCWChar},
};
