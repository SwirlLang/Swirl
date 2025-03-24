#pragma once
#include <parser/Parser.h>

struct Node;
using SwAST_t = std::vector<std::unique_ptr<Node>>;
class SymbolManager;

class AnalysisContext {
public:
    std::unordered_map<Node*, AnalysisResult> Cache;
    SymbolManager& SymMan;
    ErrorManager&  ErrMan;

    explicit AnalysisContext(Parser& parser):
        SymMan(parser.SymbolTable),
        m_AST(parser.AST),
        ErrMan(parser.ErrMan) {}

    void startAnalysis() {
        for (const auto& child : m_AST) {
            child->analyzeSemantics(*this);
        }
    }

    Type* deduceType(Type*, Type*) const;

private:
    SwAST_t& m_AST;
};