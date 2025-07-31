#include "parser/Nodes.h"


struct OpInfo {
    enum Associativity { LEFT, RIGHT};

    int precedence;
    Associativity associativity;

    OpInfo(const int prec, const Associativity associativity) : precedence(prec), associativity(associativity) {}
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

    {Op::INDEXING_OP, {200, OpInfo::LEFT}},
    {Op::DOT, {800, OpInfo::LEFT}},
};


const static
std::unordered_map<std::pair<std::string_view, int>, Op::OpTag_t> OpTagMap = {
    {{"+", 2}, Op::BINARY_ADD},
    {{"-", 2}, Op::BINARY_SUB},

    {{"+", 1}, Op::UNARY_ADD},
    {{"-", 1}, Op::UNARY_SUB},

    {{"*", 2}, Op::MUL},
    {{"/", 2}, Op::DIV},
    {{"%", 2}, Op::MOD},

    {{"!", 1},  Op::LOGICAL_NOT},
    {{"==", 2}, Op::LOGICAL_EQUAL},
    {{"!=", 2}, Op::LOGICAL_NOTEQUAL},
    {{"||", 2}, Op::LOGICAL_OR},
    {{"&&", 2}, Op::LOGICAL_AND},

    {{">", 2},  Op::GREATER_THAN},
    {{">=", 2}, Op::GREATER_THAN_OR_EQUAL},
    {{"<", 2},  Op::LESS_THAN},
    {{"<=", 2}, Op::LESS_THAN_OR_EQUAL},

    {{"[", 2},  Op::INDEXING_OP},
    {{"*", 1},  Op::DEREFERENCE},
    {{"&", 1},  Op::ADDRESS_TAKING},
    {{"as", 2}, Op::CAST_OP},

    {{".", 2},  Op::DOT},
    {{"=", 2},  Op::ASSIGNMENT},
    {{"+=", 2}, Op::ADD_ASSIGN},
    {{"-=", 2}, Op::SUB_ASSIGN},
    {{"*=", 2}, Op::MUL_ASSIGN},
    {{"/=", 2}, Op::DIV_ASSIGN},
    {{"%=", 2}, Op::MOD_ASSIGN}
};


Op::Op(const std::string_view str, const int8_t arity): value(std::string(str)), arity(arity) {
    op_type = getTagFor(str, arity);  // compute and set the tag of the operator node
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
Op::OpTag_t Op::getTagFor(const std::string_view str, int arity) {
    return OpTagMap.at({str, arity});
}

