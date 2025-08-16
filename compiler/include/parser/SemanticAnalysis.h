#pragma once
#include <stack>
#include <parser/Parser.h>

struct Node;
class SymbolManager;
class SourceManager;


class AnalysisContext {
public:
    std::unordered_map<Node*, AnalysisResult> Cache;
    std::unordered_map<IdentInfo*, Node*>& GlobalNodeJmpTable;

    SymbolManager& SymMan;
    ModuleManager& ModuleMap;
    SourceManager* SrcMan;

    ErrorCallback_t ErrCallback;

    explicit AnalysisContext(Parser& parser)
    : GlobalNodeJmpTable(parser.NodeJmpTable)
    , SymMan(parser.SymbolTable)
    , ModuleMap(parser.m_ModuleMap)
    , SrcMan(&parser.m_SrcMan)
    , ErrCallback(parser.m_ErrorCallback)
    , m_AST(parser.AST)
    {
        m_IsMethodCall.emplace(false);
        m_BoundTypeState.emplace(nullptr);
    }

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
        assert(!m_BoundTypeState.empty());
        return m_BoundTypeState.top();
    }

    void setBoundTypeState(Type* to) {
        m_BoundTypeState.emplace(to);
    }

    void restoreBoundTypeState() {
        m_BoundTypeState.pop();
    }

    void setIsMethodCallState(const bool val) {
        m_IsMethodCall.push(val);
    }

    void restoreIsMethodCallState() {
        m_IsMethodCall.pop();
    }

    bool getIsMethodCallState() const {
        assert(!m_IsMethodCall.empty());
        return m_IsMethodCall.top();
    }

    void disableErrorCode(const ErrCode code) {
        m_DisabledErrorCodes.emplace(code);
    }

    void enableErrorCode(const ErrCode code) {
        m_DisabledErrorCodes.erase(code);
    }

    void reportError(const ErrCode code, ErrorContext err_ctx) const {
        if (m_DisabledErrorCodes.contains(code))
            return;
        err_ctx.src_man = SrcMan;
        ErrCallback(code, err_ctx);
    }

    void analyzeSemanticsOf(IdentInfo* id);

    Type* deduceType(Type*, Type*, StreamState location) const;

    /// checks whether `from` can be converted to `to`, return `false` if incompatible
    bool checkTypeCompatibility(Type* from, Type* to, StreamState location) const;

    Type* getUnderlyingType(Type*);


    struct DisableErrorCode {
        DisableErrorCode(const ErrCode code, AnalysisContext& ctx): m_Ctx(ctx), m_ErrorCode(code) {
            m_Ctx.disableErrorCode(code);
        }

        ~DisableErrorCode() {
            m_Ctx.enableErrorCode(m_ErrorCode);
        }

    private:
        AnalysisContext& m_Ctx;
        ErrCode m_ErrorCode;
    };

private:
    AST_t& m_AST;

    std::stack<Type*> m_BoundTypeState;
    std::stack<bool>  m_IsMethodCall;

    Node* m_CurrentParentFunc = nullptr;
    Node* m_CurrentParentFuncCache = nullptr;

    std::unordered_set<ErrCode> m_DisabledErrorCodes;
};