#pragma once
#include <parser/Parser.h>

struct Node;
using SwAST_t = std::vector<std::unique_ptr<Node>>;
class SymbolManager;

class AnalysisContext {
public:
    std::unordered_map<Node*, AnalysisResult> Cache;

    explicit AnalysisContext(Parser& parser): SymMan(parser.SymbolTable), m_AST(parser.AST) {}

    void startAnalysis() {
        for (const auto& child : m_AST) {
            child->analyzeSemantics(*this);
        }
    }

    SymbolManager& SymMan;

private:
    SwAST_t& m_AST;
};