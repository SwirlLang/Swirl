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

    explicit AnalysisContext(Parser& parser)
    : GlobalNodeJmpTable(parser.GlobalNodeJmpTable)
    , SymMan(parser.SymbolTable)
    , ErrMan(parser.ErrMan)
    , m_AST(parser.AST) {}

    void startAnalysis() {
        for (const auto& child : m_AST) {
            child->analyzeSemantics(*this);
        } SymMan.fulfillRemainingPromises();
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

    /// fulfills all existing promises for the AR of `id`
    // void fulfillPromises(IdentInfo* id, const AnalysisResult result) {
    //     assert(id != nullptr);
    //     if (m_ARPromises.contains(id)) {
    //         for (auto& promises = m_ARPromises[id]; auto& promise : promises) {
    //             promise.set_value(result);
    //         }
    //     } m_ARPromises.erase(id);
    // }


    Type* deduceType(Type*, Type*, StreamState location) const;

    /// checks whether `from` can be converted to `to`
    void checkTypeCompatibility(Type* from, Type* to, StreamState location) const;

private:
    SwAST_t& m_AST;

    Type* m_BoundTypeState = nullptr;
    Type* m_BoundTypeStateCache = nullptr;

    Node* m_CurrentParentFunc = nullptr;
    Node* m_CurrentParentFuncCache = nullptr;

    // std::unordered_map<IdentInfo*, std::vector<std::promise<AnalysisResult>>> m_ARPromises;  // AR = AnalysisResult

    // std::future<AnalysisResult> subscribeForAnalysisResult(IdentInfo* id) {
    //     assert(id != nullptr);
    //     ScopeEndCallback _{[id, this] {
    //         if (Cache.contains(id)) {
    //             m_ARPromises[id].back().set_value(Cache[id]);
    //         }
    //     }};
    //
    //     if (m_ARPromises.contains(id)) {
    //         m_ARPromises[id].emplace_back();
    //         return m_ARPromises[id].back().get_future();
    //     }
    //
    //     m_ARPromises[id] = {std::promise<AnalysisResult>()};
    //     return m_ARPromises[id].back().get_future();
    // }
};