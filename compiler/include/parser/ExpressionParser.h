#pragma once
#include "ast/Nodes.h"


struct Node;
class Parser;
class TokenStream;


class ExpressionParser {
    Parser& m_Parser;

    Node* parseComponent();
    Node* parsePrefix();

public:
    [[nodiscard]]
    std::string_view internString(std::string_view str) const;

    explicit ExpressionParser(Parser& parser);
    Expression parseExpr(int rbp = -1);
};