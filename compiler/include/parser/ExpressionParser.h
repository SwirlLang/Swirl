#pragma once
#include <memory>

#include "ast/Nodes.h"


struct Node;
class Parser;
class TokenStream;


class ExpressionParser {
    Parser& m_Parser;

    std::unique_ptr<Node> parseComponent();
    std::unique_ptr<Node> parsePrefix();

public:
    explicit ExpressionParser(Parser& parser);
    Expression parseExpr(int rbp = -1);
};