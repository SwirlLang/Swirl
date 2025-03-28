#pragma once
#include <parser/Parser.h>

struct Node;
using SwAST_t = std::vector<std::unique_ptr<Node>>;
class SymbolManager;

class AnalysisContext {
public:
    std::unordered_map<Node*, AnalysisResult> Cache;
    std::unordered_map<IdentInfo*, Node*>& GlobalNodeJmpTable;
    Node* CurrentParentFunc = nullptr;

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

    Type* getBoundTypeState() const {
        return m_BoundTypeState;
    }

    void setBoundTypeState(Type* to) {
        m_BoundTypeStateCache = m_BoundTypeState;
        m_BoundTypeState = to;
    }

    void restoreBoundTypeState() {
        m_BoundTypeState = m_BoundTypeStateCache;
    }

    Type* deduceType(Type*, Type*) const;

private:
    SwAST_t& m_AST;
    Type* m_BoundTypeState = nullptr;
    Type* m_BoundTypeStateCache = nullptr;
};