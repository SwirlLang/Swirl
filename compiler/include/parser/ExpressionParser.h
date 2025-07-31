#pragma once
#include <memory>
#include <optional>
#include <parser/Nodes.h>

#include "managers/ErrorManager.h"

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