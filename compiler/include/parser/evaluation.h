#pragma once
#include <variant>
#include <string>
#include <cassert>


#define DEFINE_OP(op) EvalResult operator op(const EvalResult &other) { \
        assert(!isString() && !other.isString()); \
        if (other.isDouble()) { \
            if (isInt()) { return static_cast<double>(toInt()) op other.toDouble(); } \
            return toDouble() op other.toDouble(); \
        } \
        if (other.isInt()) { \
            if (isDouble()) { return static_cast<double>(other.toInt()) op toDouble(); } \
            return toInt() op  other.toInt(); \
        } \
        throw; \
    }

#define DEFINE_COMP_OP(op) EvalResult operator op(const EvalResult &other) { \
        if (other.isDouble()) { \
            if (isInt()) { return static_cast<double>(toInt()) op other.toDouble(); } \
            return toDouble() op other.toDouble(); \
        } \
        if (other.isInt()) { \
            if (isDouble()) { return static_cast<double>(other.toInt()) op toDouble(); } \
            return toInt() op  other.toInt(); \
        } \
        if (other.isBool() && isBool()) { \
            return toBool() op other.toBool(); \
        } \
        if (other.isString() && isString()) { return toString() op other.toString(); } \
        throw; \
    }



using SwEvalResultBase = std::variant<bool, double, std::size_t, std::string>;
struct EvalResult : SwEvalResultBase {
    using SwEvalResultBase::variant;

    [[nodiscard]] bool toBool() const {
        assert(isBool());
        return std::get<bool>(*this);
    }

    [[nodiscard]] double toDouble() const {
        assert(isDouble());
        return std::get<double>(*this);
    }

    [[nodiscard]] std::size_t toInt() const {
        assert(isInt());
        return std::get<std::size_t>(*this);
    }

    [[nodiscard]] std::string toString() const {
        assert(isString());
        return std::get<std::string>(*this);
    }

    [[nodiscard]] bool isInt() const {
        return std::holds_alternative<std::size_t>(*this);
    }

    [[nodiscard]] bool isBool() const {
        return std::holds_alternative<bool>(*this);
    }

    [[nodiscard]] bool isDouble() const {
        return std::holds_alternative<double>(*this);
    }

    [[nodiscard]] bool isString() const {
        return std::holds_alternative<std::string>(*this);
    }

    DEFINE_OP(+);
    DEFINE_OP(*);
    DEFINE_OP(/);
    DEFINE_OP(-);

    DEFINE_OP(<);
    DEFINE_OP(>);
    DEFINE_OP(<=);
    DEFINE_OP(>=);

    bool operator||(const EvalResult &other) const {
        if (other.isDouble()) {
            if (isInt()) { return static_cast<double>(toInt()) || other.toDouble(); }
            return toDouble() || other.toDouble();
        }
        if (other.isInt()) {
            if (isDouble()) { return static_cast<double>(other.toInt()) || toDouble(); }
            return toInt() || other.toInt();
        }
        if (isBool() && other.isBool()) {
            return toBool() || other.toBool();
        }
        throw;
    }

    bool operator&&(const EvalResult &other) const {
        if (other.isDouble()) {
            if (isInt()) { return static_cast<double>(toInt()) && other.toDouble(); }
            return toDouble() && other.toDouble();
        }
        if (other.isInt()) {
            if (isDouble()) { return static_cast<double>(other.toInt()) && toDouble(); }
            return toInt() && other.toInt();
        }
        if (isBool() && other.isBool()) {
            return toBool() && other.toBool();
        }
        throw;
    }


    DEFINE_COMP_OP(==);
    DEFINE_COMP_OP(!=);

#undef DEFINE_OP
#undef DEFINE_COMP_OP
};
