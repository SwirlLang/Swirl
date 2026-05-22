#pragma once
#include <cstdint>
#include <string_view>
#include "types/SwTypes.h"


namespace sw {
struct Value {
    enum tag { INVALID, BOOL, FLOAT, STR, UINT, INT };
    tag type = INVALID;

    union {
        bool             val_bool;
        double           val_double;
        std::int64_t     val_int;
        std::uint64_t    val_uint;
        std::string_view val_str;
    };

    static Value makeBool(const bool val) {
        Value v{.type = BOOL};
        v.val_bool = val;
        return v;
    }

    static Value makeDouble(const double val) {
        Value v{.type = FLOAT};
        v.val_double = val;
        return v;
    }

    static Value makeInt(const std::int64_t val) {
        Value v{.type = INT};
        v.val_int = val;
        return v;
    }

    static Value makeUInt(const std::uint64_t val) {
        Value v{.type = UINT};
        v.val_uint = val;
        return v;
    }

    static Value makeStr(const std::string_view val) {
        Value v{.type = STR};
        v.val_str = val;
        return v;
    }


#define NUMERIC_OP(OP) \
    friend Value operator OP(const Value& a, const Value& b) { \
        if (a.type == INT && b.type == INT) return makeInt(a.val_int OP b.val_int); \
        if (a.type == UINT && b.type == UINT) return makeUInt(a.val_uint OP b.val_uint); \
        if (a.type == FLOAT && b.type == FLOAT) return makeDouble(a.val_double OP b.val_double); \
        return {}; \
    }

    NUMERIC_OP(+)
    NUMERIC_OP(-)
    NUMERIC_OP(*)
    NUMERIC_OP(/)

#define INT_OP(OP) \
    friend Value operator OP(const Value& a, const Value& b) { \
        if (a.type == INT && b.type == INT) return makeInt(a.val_int OP b.val_int); \
        if (a.type == UINT && b.type == UINT) return makeUInt(a.val_uint OP b.val_uint); \
        return {}; \
    }

    INT_OP(%)
    INT_OP(&)
    INT_OP(|)
    INT_OP(^)

#define SHIFT_OP(OP) \
    friend Value operator OP(const Value& a, const Value& b) { \
        if (a.type == INT && b.type == INT) return makeInt(a.val_int OP b.val_int); \
        if (a.type == UINT && b.type == UINT) return makeUInt(a.val_uint OP b.val_uint); \
        return {}; \
    }

    SHIFT_OP(<<)
    SHIFT_OP(>>)

#define EQ_OP(OP) \
    friend Value operator OP(const Value& a, const Value& b) { \
        if (a.type == BOOL && b.type == BOOL) return makeBool(a.val_bool OP b.val_bool); \
        if (a.type == INT && b.type == INT) return makeBool(a.val_int OP b.val_int); \
        if (a.type == UINT && b.type == UINT) return makeBool(a.val_uint OP b.val_uint); \
        if (a.type == FLOAT && b.type == FLOAT) return makeBool(a.val_double OP b.val_double); \
        if (a.type == STR && b.type == STR) return makeBool(a.val_str OP b.val_str); \
        return makeBool(false); \
    }

#define CMP_OP(OP) \
    friend Value operator OP(const Value& a, const Value& b) { \
        if (a.type == INT && b.type == INT) return makeBool(a.val_int OP b.val_int); \
        if (a.type == UINT && b.type == UINT) return makeBool(a.val_uint OP b.val_uint); \
        if (a.type == FLOAT && b.type == FLOAT) return makeBool(a.val_double OP b.val_double); \
        return makeBool(false); \
    }

    EQ_OP(==)
    EQ_OP(!=)
    CMP_OP(<)
    CMP_OP(<=)
    CMP_OP(>)
    CMP_OP(>=)

    Value operator-() const {
        if (type == INT) return makeInt(-val_int);
        if (type == FLOAT) return makeDouble(-val_double);
        return {};
    }

    Value operator~() const {
        if (type == INT) return makeInt(~val_int);
        if (type == UINT) return makeUInt(~val_uint);
        return {};
    }

    Value operator!() const {
        if (type == BOOL) return makeBool(!val_bool);
        return {};
    }
};

#undef CMP_OP
#undef EQ_OP
#undef SHIFT_OP
#undef INT_OP
#undef NUMERIC_OP
}
