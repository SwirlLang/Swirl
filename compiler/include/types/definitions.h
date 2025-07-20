#pragma once
#include <string_view>
#include <unordered_map>

#include "SwTypes.h"


inline TypeI8 GlobalTypeI8{};
inline TypeU8 GlobalTypeU8{};

inline TypeI16 GlobalTypeI16{};
inline TypeU16 GlobalTypeU16{};

inline TypeI32 GlobalTypeI32{};
inline TypeU32 GlobalTypeU32{};

inline TypeI64 GlobalTypeI64{};
inline TypeU64 GlobalTypeU64{};

inline TypeI128 GlobalTypeI128{};
inline TypeU128 GlobalTypeU128{};

inline TypeF32 GlobalTypeF32{};
inline TypeF64 GlobalTypeF64{};

inline TypeBool GlobalTypeBool{};
inline TypeStr  GlobalTypeStr{};

inline VoidType GlobalTypeVoid{};



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
    {"str",  &GlobalTypeStr},

    {"void", &GlobalTypeVoid},
};
