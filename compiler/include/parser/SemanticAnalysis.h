#pragma once
#include <stack>
#include <parser/Parser.h>

struct Node;
class SymbolManager;
class SourceManager;


/// This class allows the calls of `analyzeSemantics` to remain in sync with each
/// other and have stateful behavior. An instance of this class is created in
/// association with a `Parser` instance, `startAnalysis` is then called to begin sema.
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

    /// Begins semantic analysis
    void startAnalysis() {
        for (const auto& child : m_AST) {
            child->analyzeSemantics(*this);
        }
    }

    /// Returns the last-entered function-node pointer
    Node* getCurParentFunc() const {
        return m_CurrentParentFunc;
    }

    /// Sets the current function node pointer (CFNP) to `to`
    void setCurParentFunc(Node* to) {
        m_CurrentParentFuncCache = m_CurrentParentFunc;
        m_CurrentParentFunc = to;
    }

    /// Restores the CFNP state
    void restoreCurParentFunc() {
        m_CurrentParentFunc = m_CurrentParentFuncCache;
    }


    // --- --- --- [BoundTypeState] --- --- --- //
    // The `BoundTypeState` is used to pass down information about the currently
    // expected type. This is set by nodes like Variables, Functions, etc.
    // which have can have any explicitly-written types, to let its constituents
    // (say expressions) know of the type they are expected to be in.
    //
    // E.g. in `var a: i16 = 4`, since the variable has an explicitly-written type
    // `i16`, it sets this state to `i16`, to let the constituents of the
    // expression (IntLit) know that they are expected to be of this type.

    /// Returns the current value of the state
    Type* getBoundTypeState() const {
        assert(!m_BoundTypeState.empty());
        return m_BoundTypeState.top();
    }

    /// Changes the value of the state to `to`
    void setBoundTypeState(Type* to) {
        m_BoundTypeState.emplace(to);
    }

    /// Restores the state to its previous value
    void restoreBoundTypeState() {
        m_BoundTypeState.pop();
    }

    // --- --- --- [IsMethodCallState] --- --- --- //
    // This state is used to let know `FuncCall::analyzeSemantics` whether
    // it's a regular function call or a method-call.

    /// Returns the current value of the state
    bool getIsMethodCallState() const {
        assert(!m_IsMethodCall.empty());
        return m_IsMethodCall.top();
    }

    /// Changes the value of the state to `to`
    void setIsMethodCallState(const bool val) {
        m_IsMethodCall.push(val);
    }

    /// Restores the state to its previous value
    void restoreIsMethodCallState() {
        m_IsMethodCall.pop();
    }

    /// Makes `reportError` ignore reports with this error code
    void disableErrorCode(const ErrCode code) {
        m_DisabledErrorCodes.emplace(code);
    }

    /// Undoes what the above does
    void enableErrorCode(const ErrCode code) {
        m_DisabledErrorCodes.erase(code);
    }

    /// Convenient wrapper of the error callback
    void reportError(const ErrCode code, ErrorContext err_ctx) const {
        if (m_DisabledErrorCodes.contains(code))
            return;
        err_ctx.src_man = SrcMan;
        ErrCallback(code, err_ctx);
    }


    /// Calls `analyzeSemantics` on the node with an id = `id`
    void analyzeSemanticsOf(IdentInfo* id);

    /// Returns the common-type of `type1` and `type2`, nullptr if failure
    Type* deduceType(Type* type1, Type* type2, StreamState location) const;

    /// Checks whether `from` can be converted to `to`, returns `false` if incompatible
    bool checkTypeCompatibility(Type* from, Type* to, StreamState location) const;


    /// Disables the error code in its constructor, enables it back in its destructor
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