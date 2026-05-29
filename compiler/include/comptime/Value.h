#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <functional>


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


#define CMP_OP(OP) \
    friend Value operator OP(const Value& a, const Value& b) { \
        if (a.type == INT && b.type == INT) return makeBool(a.val_int OP b.val_int); \
        if (a.type == UINT && b.type == UINT) return makeBool(a.val_uint OP b.val_uint); \
        if (a.type == FLOAT && b.type == FLOAT) return makeBool(a.val_double OP b.val_double); \
        return makeBool(false); \
    }

    CMP_OP(<)
    CMP_OP(<=)
    CMP_OP(>)
    CMP_OP(>=)

    bool operator==(const Value& other) const {
        if (type != other.type) return false;
        switch (type) {
            case BOOL:  return val_bool == other.val_bool;
            case FLOAT: return val_double == other.val_double;
            case INT:   return val_int == other.val_int;
            case UINT:  return val_uint == other.val_uint;
            case STR:   return val_str == other.val_str;
            default:    return false;
        }
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

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

    [[nodiscard]]
    std::string toString() const {
        switch (type) {
            case BOOL:  return val_bool ? "true" : "false";
            case FLOAT: return std::to_string(val_double);
            case INT:   return std::to_string(val_int);
            case UINT:  return std::to_string(val_uint);
            case STR:   return '"' + std::string(val_str) + '"';
            default:    return "invalid";
        }
    }
};

#undef CMP_OP
#undef SHIFT_OP
#undef INT_OP
#undef NUMERIC_OP
}


template <>
struct std::hash<sw::Value> {
    std::size_t operator()(const sw::Value& v) const noexcept {
        switch (v.type) {
            case sw::Value::BOOL:  return std::hash<bool>{}(v.val_bool);
            case sw::Value::FLOAT: return std::hash<double>{}(v.val_double);
            case sw::Value::INT:   return std::hash<std::int64_t>{}(v.val_int);
            case sw::Value::UINT:  return std::hash<std::uint64_t>{}(v.val_uint);
            case sw::Value::STR:   return std::hash<std::string_view>{}(v.val_str);
            default:               return std::hash<int>{}(v.type);
        }
    }
};
