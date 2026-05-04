#pragma once
#include <memory>
#include <memory_resource>

#include "ast/Nodes.h"


struct Node;
class Parser;
class TokenStream;


class ExpressionParser {
    Parser& m_Parser;

    Node* parseComponent();
    Node* parsePrefix();

public:
    explicit ExpressionParser(Parser& parser);
    Expression parseExpr(int rbp = -1);
};