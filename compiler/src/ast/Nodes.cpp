#include "ast/Nodes.h"
#include "parser/Parser.h"


struct OpInfo {
    enum Associativity { LEFT, RIGHT };

    int precedence;
    Associativity associativity;

    OpInfo(const int prec, const Associativity ass) : precedence(prec), associativity(ass) {}
};


struct PairHash {
    std::size_t operator()(const std::pair<Token::TokenValue, int>& pair) const noexcept {
        return combineHashes(std::hash<int>()(pair.first), std::hash<int>()(pair.second));
    }
};


const static
std::unordered_map<Op::OpTag_t, OpInfo> OpInfoTable = {
    {Op::ASSIGNMENT, {0, OpInfo::RIGHT}},
    {Op::ADD_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::MUL_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::SUB_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::DIV_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::MOD_ASSIGN, {0, OpInfo::RIGHT}},


    {Op::LOGICAL_OR, {10, OpInfo::LEFT}},
    {Op::LOGICAL_AND, {20, OpInfo::LEFT}},
    {Op::LOGICAL_EQUAL, {30, OpInfo::LEFT}},
    {Op::LOGICAL_NOTEQUAL, {30, OpInfo::LEFT}},

    {Op::GREATER_THAN, {40, OpInfo::LEFT}},
    {Op::GREATER_THAN_OR_EQUAL, {40, OpInfo::LEFT}},
    {Op::LESS_THAN, {40, OpInfo::LEFT}},
    {Op::LESS_THAN_OR_EQUAL, {40, OpInfo::LEFT}},


    {Op::BINARY_ADD, {50, OpInfo::LEFT}},
    {Op::BINARY_SUB, {50, OpInfo::LEFT}},

    {Op::MUL, {60, OpInfo::LEFT}},
    {Op::DIV, {60, OpInfo::LEFT}},
    {Op::MOD, {60, OpInfo::LEFT}},

    {Op::CAST_OP, {100, OpInfo::LEFT}},

    {Op::ADDRESS_TAKING, {150, OpInfo::LEFT}},
    {Op::UNARY_ADD, {150, OpInfo::LEFT}},
    {Op::UNARY_SUB, {150, OpInfo::LEFT}},
    {Op::DEREFERENCE, {150, OpInfo::LEFT}},

    {Op::INDEXING_OP, {200, OpInfo::LEFT}},
    {Op::DOT, {800, OpInfo::LEFT}},

    {Op::BITWISE_LSHIFT, {45, OpInfo::LEFT}},
    {Op::BITWISE_RSHIFT, {45, OpInfo::LEFT}},
    {Op::BITWISE_AND, {28, OpInfo::LEFT}},
    {Op::BITWISE_XOR, {26, OpInfo::LEFT}},
    {Op::BITWISE_OR, {24, OpInfo::LEFT}},

    {Op::BITWISE_NOT, {150, OpInfo::LEFT}},

    {Op::BITWISE_OR_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::BITWISE_AND_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::BITWISE_XOR_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::BITWISE_LSHIFT_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::BITWISE_RSHIFT_ASSIGN, {0, OpInfo::RIGHT}},
};


const static
std::unordered_map<std::pair<Token::TokenValue, int>, Op::OpTag_t, PairHash> OpTagMap = {
    {{Token::OP_PLUS, 2}, Op::BINARY_ADD},
    {{Token::OP_MINUS, 2}, Op::BINARY_SUB},

    {{Token::OP_PLUS, 1}, Op::UNARY_ADD},
    {{Token::OP_MINUS, 1}, Op::UNARY_SUB},

    {{Token::OP_MUL, 2}, Op::MUL},
    {{Token::OP_DIV, 2}, Op::DIV},
    {{Token::OP_MOD, 2}, Op::MOD},

    {{Token::OP_NOT, 1},  Op::LOGICAL_NOT},
    {{Token::OP_EQ, 2}, Op::LOGICAL_EQUAL},
    {{Token::OP_NOT_EQ, 2}, Op::LOGICAL_NOTEQUAL},
    {{Token::OP_LOGICAL_OR, 2}, Op::LOGICAL_OR},
    {{Token::OP_LOGICAL_AND, 2}, Op::LOGICAL_AND},

    {{Token::OP_GT, 2},  Op::GREATER_THAN},
    {{Token::OP_GT_EQ, 2}, Op::GREATER_THAN_OR_EQUAL},
    {{Token::OP_LT, 2},  Op::LESS_THAN},
    {{Token::OP_LT_EQ, 2}, Op::LESS_THAN_OR_EQUAL},

    {{Token::PUNC_LBRACKET, 2},  Op::INDEXING_OP},
    {{Token::OP_MUL, 1},  Op::DEREFERENCE},
    {{Token::OP_BITWISE_AND, 1},  Op::ADDRESS_TAKING},
    {{Token::OP_AS, 2}, Op::CAST_OP},

    {{Token::OP_DOT, 2},  Op::DOT},
    {{Token::OP_ASSIGN, 2},  Op::ASSIGNMENT},
    {{Token::OP_PLUS_ASSIGN, 2}, Op::ADD_ASSIGN},
    {{Token::OP_MINUS_ASSIGN, 2}, Op::SUB_ASSIGN},
    {{Token::OP_MUL_ASSIGN, 2}, Op::MUL_ASSIGN},
    {{Token::OP_DIV_ASSIGN, 2}, Op::DIV_ASSIGN},
    {{Token::OP_MOD_ASSIGN, 2}, Op::MOD_ASSIGN},

    {{Token::OP_BITWISE_OR, 2}, Op::BITWISE_OR},
    {{Token::OP_BITWISE_AND, 2}, Op::BITWISE_AND},
    {{Token::OP_XOR, 2}, Op::BITWISE_XOR},
    {{Token::OP_BITWISE_NOT, 1}, Op::BITWISE_NOT},
    {{Token::OP_LBITSHIFT, 2}, Op::BITWISE_LSHIFT},
    {{Token::OP_RBITSHIFT, 2}, Op::BITWISE_RSHIFT},

    {{Token::OP_BITWISE_OR_ASSIGN, 2}, Op::BITWISE_OR_ASSIGN},
    {{Token::OP_BITWISE_AND_ASSIGN, 2}, Op::BITWISE_AND_ASSIGN},
    {{Token::OP_XOR_ASSIGN, 2}, Op::BITWISE_XOR_ASSIGN},
    {{Token::OP_LBITSHIFT_ASSIGN, 2}, Op::BITWISE_LSHIFT_ASSIGN},
    {{Token::OP_RBITSHIFT_ASSIGN, 2}, Op::BITWISE_RSHIFT_ASSIGN}
};


Op::Op(const Token::TokenValue id, const int8_t adicity): Op()
{
    arity = adicity;
    tokenid = id;
    op_type = getTagFor(id, adicity);  // compute and set the tag of the operator node
}


/// Returns the Left-Binding Power of the operator with the tag `op`.
int Op::getLBPFor(const OpTag_t op) {
    return OpInfoTable.at(op).precedence;
}


/// Returns the Right-Binding Power of the operator with the tag `op`.
int Op::getRBPFor(const OpTag_t op) {
    const auto info = OpInfoTable.at(op);
    return info.associativity == OpInfo::RIGHT ? info.precedence - 1 : info.precedence;
}


/// Returns the Prefix-Binding Power of the operator with the tag `op`.
int Op::getPBPFor(const OpTag_t op) {
    const auto info = OpInfoTable.at(op);
    return info.precedence;
}


/// Returns a tag which uniquely identifies the operator, given its string-representation and arity.
Op::OpTag_t Op::getTagFor(const Token::TokenValue tok, int arity) {
    return OpTagMap.at({tok, arity});
}


void Op::setType(Type* to) const {
    if (op_type == DOT)
        return;

    if (operands.front()->getNodeType() == ND_EXPR) {
        dynamic_cast<Expression*>(operands.front())->setType(to);
    }

    if (arity == 1) return;
    if (operands.back()->getNodeType() == ND_EXPR) {
        auto expr = dynamic_cast<Expression*>(operands.back());
        expr->setType(to);
        return;
    }
}

void Expression::setType(Type* to) {
    expr_type = to;
    if (expr->getNodeType() == ND_EXPR) {
        dynamic_cast<Expression*>(expr)->setType(to);
    }
    else if (expr->getNodeType() == ND_OP) {
        dynamic_cast<Op*>(expr)->setType(to);
    }
}


/// Constructs and returns an expression out of the `EvalResult` variant
[[deprecated]]
Expression Expression::makeExpression(const EvalResult& e) {
    return std::visit(
    VisitorHelper{
        [](std::monostate) { return makeExpression(nullptr); },
        [](const bool v) { return makeExpression(new BoolLit{v}); },
        [](const double v) { return makeExpression(new FloatLit{"888"}); },
        [](const std::size_t v) { return makeExpression(new IntLit("888")); },
        [](const std::string& v) { return makeExpression(new StrLit{v}); },
    }, e);
}


EvalResult Node::evaluate(Parser& ctx) {
    ctx.reportError(ErrCode::NOT_ALLOWED_CT_CTX);
    return {};
}

EvalResult IntLit::evaluate(Parser&) {
    // return toInteger(value);
    return {};
}

EvalResult FloatLit::evaluate(Parser&) {
    // return std::stod(value);
    return {};
}

EvalResult BoolLit::evaluate(Parser&) {
   // return value;
    return {};
}

EvalResult StrLit::evaluate(Parser&) {
   // return value;
    return {};
}


EvalResult Ident::evaluate(Parser& ctx) {
    // if (const auto node = ctx.SymbolTable.lookupDecl(getIdentInfo()).node_ptr) {
    //     if (node->getNodeType() == ND_VAR) {
    //         const auto var = dynamic_cast<Var*>(node);
    //
    //         // only allow config variables
    //         if (!var->is_comptime) {
    //             ctx.reportError(ErrCode::NOT_A_CT_VAR);
    //             return {};
    //         } return var->value.evaluate(ctx);
    //     } ctx.reportError(ErrCode::NOT_ALLOWED_CT_CTX);
    // }
    return {};
}


EvalResult Expression::evaluate(Parser& ctx) {
    return expr->evaluate(ctx);
}


EvalResult Op::evaluate(Parser& ctx) {
    switch (op_type) {
        case Op::MUL:
            return operands.at(0)->evaluate(ctx) * operands.at(1)->evaluate(ctx);
        case Op::DIV:
            return operands.at(0)->evaluate(ctx) / operands.at(1)->evaluate(ctx);
        case Op::BINARY_ADD:
            return operands.at(0)->evaluate(ctx) + operands.at(1)->evaluate(ctx);
        case Op::BINARY_SUB:
            return operands.at(0)->evaluate(ctx) - operands.at(1)->evaluate(ctx);
        case Op::GREATER_THAN:
            return operands.at(0)->evaluate(ctx) > operands.at(1)->evaluate(ctx);
        case Op::GREATER_THAN_OR_EQUAL:
            return operands.at(0)->evaluate(ctx) >= operands.at(1)->evaluate(ctx);
        case Op::LESS_THAN:
            return operands.at(0)->evaluate(ctx) < operands.at(1)->evaluate(ctx);
        case Op::LESS_THAN_OR_EQUAL:
            return operands.at(0)->evaluate(ctx) <= operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_OR:
            return operands.at(0)->evaluate(ctx) || operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_AND:
            return operands.at(0)->evaluate(ctx) && operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_EQUAL:
            return operands.at(0)->evaluate(ctx) == operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_NOTEQUAL:
            return !(operands.at(0)->evaluate(ctx) == operands.at(1)->evaluate(ctx)).toBool();
        default:
            // ctx.reportError(ErrCode::OP_NOT_ALLOWED_HERE, {.str_1 = value});
            return {};
    }
}
