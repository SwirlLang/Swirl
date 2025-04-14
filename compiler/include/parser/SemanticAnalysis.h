#pragma once
#include <parser/Parser.h>

struct Node;
using SwAST_t = std::vector<std::unique_ptr<Node>>;
class SymbolManager;


class AnalysisContext {
public:
    std::unordered_map<Node*, AnalysisResult> Cache;
    std::unordered_map<IdentInfo*, Node*>& GlobalNodeJmpTable;

    /// Maps module names (or their aliases) to their path, used for the resolution of  ///
    /// identifiers' `IdentInfo*`s.                                                    ///
    std::unordered_map<std::string, std::filesystem::path> ModuleNamespaceTable;

    SymbolManager& SymMan;
    ErrorManager&  ErrMan;

    explicit AnalysisContext(Parser& parser)
    : GlobalNodeJmpTable(parser.GlobalNodeJmpTable)
    , SymMan(parser.SymbolTable)
    , ErrMan(parser.ErrMan)
    , m_AST(parser.AST) {}

    void startAnalysis() {
        for (const auto& child : m_AST) {
            child->analyzeSemantics(*this);
        }
    }

    Node* getCurParentFunc() const {
        return m_CurrentParentFunc;
    }

    void setCurParentFunc(Node* to) {
        m_CurrentParentFuncCache = m_CurrentParentFunc;
        m_CurrentParentFunc = to;
    }

    void restoreCurParentFunc() {
        m_CurrentParentFunc = m_CurrentParentFuncCache;
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

    void analyzeSemanticsOf(IdentInfo* id);

    Type* deduceType(Type*, Type*, StreamState location) const;

    /// checks whether `from` can be converted to `to`
    void checkTypeCompatibility(Type* from, Type* to, StreamState location) const;

private:
    SwAST_t& m_AST;

    Type* m_BoundTypeState = nullptr;
    Type* m_BoundTypeStateCache = nullptr;

    Node* m_CurrentParentFunc = nullptr;
    Node* m_CurrentParentFuncCache = nullptr;
};