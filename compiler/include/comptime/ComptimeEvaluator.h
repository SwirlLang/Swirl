#pragma once
#include <cmath>
#include <utility>

#include "Value.h"
#include "ast/TransformVisitor.h"


namespace sw {
class ComptimeEvaluator : public TransformVisitor<ComptimeEvaluator> {
public:
    explicit ComptimeEvaluator(Module* module, ErrorCallback_t error_callback)
        : TransformVisitor(module)
        , m_Module(module)
        , m_ErrorCallback(std::move(error_callback)) {}


    struct Context {
        Type* type = nullptr;
    };


    Value evaluate(const Node* node, const Context ctx) {
    #define SW_NODE(x, y) case x: return compute(static_cast<const y*>(node), ctx);
        switch (node->kind) {
            SW_NODE_LIST;
            default: throw std::runtime_error("ComptimeEvaluator::evaluate(): invalid node kind");
        }
    #undef SW_NODE
    }


    Node* transform(const Ident* node) {
        assert(node->value);

        if (const TableEntry* decl = SymMan.searchDecl(node->value)) {
            if (decl->is_comptime) {
                assert(decl->node_ptr);
                return makeNode(evaluate(decl->node_ptr, {}));
            }
        }

        return const_cast<Ident*>(node);
    }


    Node* transform(const Expression* node) {
        if (node->is_comptime) {
            assert(node->expr);
            return makeNode(evaluate(node->expr, {.type = node->expr_type}));
        } return const_cast<Node*>(transformDefault(node));
    }


private:
    Module* m_Module;
    ErrorCallback_t m_ErrorCallback;
    bool m_ErrorsOccurred = false;


    [[nodiscard]]
    Node* makeNode(Value val) const {
    #define make_expr(x) m_Module->makeNode<Expression>(Expression::makeExpression(x));
        switch (val.type) {
            case Value::STR:
                return make_expr(m_Module->makeNode<StrLit>(val.val_str));
            case Value::UINT:
                return make_expr(m_Module->makeNode<IntLit>(
                    internStr(std::to_string(val.val_uint))));
            case Value::INT:
                return make_expr(m_Module->makeNode<IntLit>(
                    internStr(std::to_string(val.val_int))));
            case Value::FLOAT:
                return make_expr(m_Module->makeNode<FloatLit>(
                    internStr(std::to_string(val.val_double))));
            case Value::BOOL:
                return make_expr(m_Module->makeNode<BoolLit>(val.val_bool));
            default:
                return nullptr;
        }
        #undef make_expr
    }


    Value compute(const Var* node, Context ctx) {
        return compute(node->value, ctx);
    }


    Value compute(const Expression* node, const Context ctx) {
        return evaluate(node->expr, {.type = node->expr_type});
    }


    Value compute(const IntLit* node, const Context ctx) {
        if (ctx.type && ctx.type->isUnsigned())
            return Value::makeUInt(toUInt64(node->value));
        return Value::makeInt(toInt64(node->value));
    }


    Value compute(const FloatLit* node, Context ctx) {
        return Value::makeDouble(std::stod(std::string(node->value)));
    }


    Value compute(const BoolLit* node, Context ctx) {
        return Value::makeBool(node->value);
    }


    Value compute(const StrLit* node, Context ctx) {
        return Value::makeStr(node->value);
    }


    Value compute(const Ident* node, const Context ctx) {
        assert(node->value);

        if (node->value->isFictitious()) {
            const auto enum_node = SymMan.getFictitiousIDValue(node->value);
            return Value{
                .type    = Value::INT,
                .val_int = enum_node->entries.at(node->value->toString())
            };
        }

        const TableEntry decl = SymMan.lookupDecl(node->value);

        if (!decl.is_comptime) {
            reportError(ErrCode::NOT_ALLOWED_CT_CTX, {
                .location = node->location});
            return {};
        }

        return evaluate(decl.node_ptr, ctx);
    }


    Value compute(const Op* node, Context ctx) {
        if (node->op_type == Op::CAST_OP) {
            Value src = evaluate(node->operands.at(0), ctx);
            if (src.type == Value::INVALID) return {};
            return castValue(src, node->operands.at(1)->getSwType());
        }

        const Value operand_1 = evaluate(node->operands.at(0), ctx);
        const Value operand_2 = node->arity == 2 ? evaluate(node->operands.at(1), ctx) : Value{};

        if (operand_1.type == Value::INVALID || (node->arity == 2 && operand_2.type == Value::INVALID)) {
            return Value{};
        }

        auto to_double = [](const Value& v) -> double {
            switch (v.type) {
                case Value::INT:   return static_cast<double>(v.val_int);
                case Value::UINT:  return static_cast<double>(v.val_uint);
                case Value::FLOAT: return v.val_double;
                default:           return 0.0;
            }
        };

        switch (node->op_type) {
            case Op::BINARY_ADD:      return operand_1 + operand_2;
            case Op::BINARY_SUB:      return operand_1 - operand_2;
            case Op::MUL:             return operand_1 * operand_2;
            case Op::DIV:             return operand_1 / operand_2;
            case Op::MOD:             return operand_1 % operand_2;
            case Op::EXP: {
                const double r = std::pow(to_double(operand_1), to_double(operand_2));
                if (operand_1.type == Value::INT) return Value::makeInt(static_cast<std::int64_t>(r));
                if (operand_1.type == Value::UINT) return Value::makeUInt(static_cast<std::uint64_t>(r));
                return Value::makeDouble(r);
            }

            case Op::UNARY_ADD:               return operand_1;
            case Op::UNARY_SUB:               return -operand_1;
            case Op::LOGICAL_NOT:             return !operand_1;
            case Op::BITWISE_NOT:             return ~operand_1;
            case Op::BITWISE_OR:              return operand_1 | operand_2;
            case Op::BITWISE_AND:             return operand_1 & operand_2;
            case Op::BITWISE_XOR:             return operand_1 ^ operand_2;
            case Op::BITWISE_LSHIFT:          return operand_1 << operand_2;
            case Op::BITWISE_RSHIFT:          return operand_1 >> operand_2;
            case Op::GREATER_THAN:            return operand_1 > operand_2;
            case Op::GREATER_THAN_OR_EQUAL:   return operand_1 >= operand_2;
            case Op::LESS_THAN:               return operand_1 < operand_2;
            case Op::LESS_THAN_OR_EQUAL:      return operand_1 <= operand_2;

            case Op::LOGICAL_EQUAL:           return Value::makeBool(operand_1 == operand_2);
            case Op::LOGICAL_NOTEQUAL:        return Value::makeBool(operand_1 != operand_2);

            case Op::LOGICAL_AND:   return Value::makeBool(operand_1.val_bool && operand_2.val_bool);
            case Op::LOGICAL_OR:    return Value::makeBool(operand_1.val_bool || operand_2.val_bool);

            default:
                reportError(ErrCode::NOT_ALLOWED_CT_CTX, {
                    .location = node->location});
                return {};
        }
    }


    Value compute(const Node* node, Context) {
        reportError(ErrCode::NOT_ALLOWED_CT_CTX, {.location = node->location});
        return {};
    }


    static Value castValue(const Value& src, Type* target) {
        auto tag_of = [](Type* t) -> Value::tag {
            if (t->isBoolean()) return Value::BOOL;
            if (t->isFloatingPoint()) return Value::FLOAT;
            if (t->isIntegral()) return t->isUnsigned() ? Value::UINT : Value::INT;
            return Value::INVALID;
        };

        const Value::tag dst = tag_of(target);
        if (dst == Value::INVALID) return {};

        switch (dst) {
            case Value::BOOL:
                switch (src.type) {
                    case Value::BOOL:  return Value::makeBool(src.val_bool);
                    case Value::INT:   return Value::makeBool(src.val_int != 0);
                    case Value::UINT:  return Value::makeBool(src.val_uint != 0);
                    case Value::FLOAT: return Value::makeBool(src.val_double != 0.0);
                    default: return {};
                }
            case Value::INT:
                switch (src.type) {
                    case Value::BOOL:  return Value::makeInt(src.val_bool ? 1 : 0);
                    case Value::INT:   return Value::makeInt(src.val_int);
                    case Value::UINT:  return Value::makeInt(static_cast<std::int64_t>(src.val_uint));
                    case Value::FLOAT: return Value::makeInt(static_cast<std::int64_t>(src.val_double));
                    default: return {};
                }
            case Value::UINT:
                switch (src.type) {
                    case Value::BOOL:  return Value::makeUInt(src.val_bool ? 1 : 0);
                    case Value::INT:   return Value::makeUInt(static_cast<std::uint64_t>(src.val_int));
                    case Value::UINT:  return Value::makeUInt(src.val_uint);
                    case Value::FLOAT: return Value::makeUInt(static_cast<std::uint64_t>(src.val_double));
                    default: return {};
                }
            case Value::FLOAT:
                switch (src.type) {
                    case Value::BOOL:  return Value::makeDouble(src.val_bool ? 1.0 : 0.0);
                    case Value::INT:   return Value::makeDouble(static_cast<double>(src.val_int));
                    case Value::UINT:  return Value::makeDouble(static_cast<double>(src.val_uint));
                    case Value::FLOAT: return Value::makeDouble(src.val_double);
                    default: return {};
                }
            default: return {};
        }
    }


    void reportError(const ErrCode code, ErrorContext ctx) {
        m_ErrorsOccurred = true;
        ctx.module = m_Module;
        m_ErrorCallback(code, ctx);
    }


    [[nodiscard]]
    std::string_view internStr(const std::string& str) const {
        return m_Module->getStringPool().internLocked(str);
    }


public:
    [[nodiscard]]
    bool errorsOccurred() const {
        return m_ErrorsOccurred;
    }

    static std::int64_t toInt64(const std::string_view s) {
        if (s.empty()) return 0;

        std::size_t i = 0;
        bool neg = false;
        if (s[i] == '-') { neg = true; ++i; }
        else if (s[i] == '+') { ++i; }

        int base = 10;
        if (i + 2 <= s.size() && s[i] == '0') {
            char p = s[i + 1];
            if (p == 'b' || p == 'B') { base = 2; i += 2; }
            else if (p == 'x' || p == 'X') { base = 16; i += 2; }
            else if (p == 'o' || p == 'O') { base = 8; i += 2; }
        }

        std::int64_t val = 0;
        for (; i < s.size(); ++i) {
            char c = s[i];
            int d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = 10 + c - 'a';
            else if (c >= 'A' && c <= 'F') d = 10 + c - 'A';
            else break;
            if (d >= base) break;
            val = val * base + d;
        }

        return neg ? -val : val;
    }


    static std::uint64_t toUInt64(const std::string_view s) {
        if (s.empty()) return 0;

        std::size_t i = 0;
        if (s[i] == '+') ++i;

        int base = 10;
        if (i + 2 <= s.size() && s[i] == '0') {
            char p = s[i + 1];
            if (p == 'b' || p == 'B') { base = 2; i += 2; }
            else if (p == 'x' || p == 'X') { base = 16; i += 2; }
            else if (p == 'o' || p == 'O') { base = 8; i += 2; }
        }

        std::uint64_t val = 0;
        for (; i < s.size(); ++i) {
            char c = s[i];
            int d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = 10 + c - 'a';
            else if (c >= 'A' && c <= 'F') d = 10 + c - 'A';
            else break;
            if (d >= base) break;
            val = val * base + d;
        }

        return val;
    }
};
}
