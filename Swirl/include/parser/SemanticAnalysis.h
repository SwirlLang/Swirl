#pragma once
#include <parser/Parser.h>

struct Node;
using SwAST_t = std::vector<std::unique_ptr<Node>>;
class SymbolManager;

class AnalysisContext {
public:
    std::unordered_map<Node*, AnalysisResult> Cache;
    std::unordered_map<IdentInfo*, Node*>& GlobalNodeJmpTable;

    SymbolManager& SymMan;
    ErrorManager&  ErrMan;

    explicit AnalysisContext(Parser& parser):
        GlobalNodeJmpTable(parser.GlobalNodeJmpTable),
        SymMan(parser.SymbolTable),
        ErrMan(parser.ErrMan),
        m_AST(parser.AST) {}

    void startAnalysis() {
        for (const auto& child : m_AST) {
            child->analyzeSemantics(*this);
        }
    }

    Type* deduceType(Type*, Type*) const;

private:
    SwAST_t& m_AST;
};