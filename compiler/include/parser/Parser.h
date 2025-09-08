#pragma once
#include <filesystem>
#include <memory>
#include <mutex>
#include <concepts>
#include <unordered_set>
#include <utility>

#include "definitions.h"
#include "parser/Nodes.h"
#include "lexer/TokenStream.h"
#include "lexer/Tokens.h"
#include "errors/ErrorManager.h"
#include "symbols/SymbolManager.h"

#include "ExpressionParser.h"


class Parser;
class ModuleManager;
class AnalysisContext;
using ErrorCallback_t = std::function<void (ErrCode, ErrorContext)>;


/// A type which can represent either a `Type*` or a `Node*`.
struct SwObject : std::variant<Type*, Node*> {
    using std::variant<Type*, Node*>::variant;

    [[nodiscard]] Type* toType() const {
        return std::get<Type*>(*this);
    }

    [[nodiscard]] Node* toNode() const {
        return std::get<Node*>(*this);
    }

    [[nodiscard]] bool isType() const {
        return std::holds_alternative<Type*>(*this);
    }

    [[nodiscard]] bool isNode() const {
        return std::holds_alternative<Node*>(*this);
    }
};


template <>
struct std::hash<SwObject> {
    std::size_t operator()(const SwObject& obj) const noexcept {
        return std::hash<std::variant<Type*, Node*>>{}(obj);
    }
};


class Parser {
    TokenStream    m_Stream;
    SourceManager  m_SrcMan;
    ModuleManager& m_ModuleMap;

    ErrorCallback_t m_ErrorCallback;  // the callback for reporting an error
    ExpressionParser m_ExpressionParser{*this};

    // ---*--- Flags  ---*---
    Function*    m_LatestFuncNode = nullptr;
    bool         m_LastSymWasExported = false;
    bool         m_LastSymIsExtern  = false;
    bool         m_IsMainModule     = false;    // is the module the parser represents the main one?
    Type*        m_CurrentStructTy  = nullptr;  // the type of the struct being parsed

    std::string m_ExternAttributes;
    std::optional<Token> m_ReturnFakeToken = std::nullopt;
    // ---*--- ---*--- ---*---

    std::size_t           m_UnresolvedDeps{};  // counter for the no. of unresolved dependencies
    std::vector<SwObject> m_ParseStack;        // currently-being-parsed object pointer stays at the top

    std::filesystem::path m_FilePath;

    std::unordered_set<Parser*> m_Dependents;     // the modules which depend on this module
    std::unordered_set<Parser*> m_Dependencies;  // the modules which this module depends on

    // used for buffering error reports until the nodes/types have been completed
    std::unordered_map<SwObject, std::vector<std::tuple<ErrCode, ErrorContext>>> m_ErrorQueue;

    struct Bracket_t { char val{}; StreamState location; };
    std::vector<Bracket_t> m_BracketTracker;

    struct NodeAttrHelper;

    friend class CompilerInst;
    friend class ModuleManager;
    friend class LLVMBackend;
    friend class AnalysisContext;
    friend class ExpressionParser;


public:
    SymbolManager SymbolTable;

    AST_t AST;
    std::unordered_map<IdentInfo*, Node*> NodeJmpTable;  // maps global symbols to their nodes

    explicit Parser(const std::filesystem::path& path, ErrorCallback_t, ModuleManager&);

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    Parser(Parser&& other) noexcept
    : m_Stream(std::move(other.m_Stream))
    , m_SrcMan(std::move(other.m_SrcMan))
    , m_ModuleMap(other.m_ModuleMap)
    , m_ErrorCallback(std::move(other.m_ErrorCallback))
    , m_LatestFuncNode(other.m_LatestFuncNode)
    , m_LastSymWasExported(other.m_LastSymWasExported)
    , m_UnresolvedDeps(other.m_UnresolvedDeps)
    , m_FilePath(std::move(other.m_FilePath))
    , m_Dependents(std::move(other.m_Dependents))
    , m_Dependencies(std::move(other.m_Dependencies))
    , SymbolTable(std::move(other.SymbolTable))
    , AST(std::move(other.AST))
    , NodeJmpTable(std::move(other.NodeJmpTable))
    {}


    std::unique_ptr<Node> dispatch();
    std::unique_ptr<Function> parseFunction();
    std::unique_ptr<Condition> parseCondition();
    std::unique_ptr<WhileLoop> parseWhile();
    std::unique_ptr<ReturnStatement> parseRet();
    std::unique_ptr<Struct> parseStruct();
    std::unique_ptr<ImportNode> parseImport();

    std::unique_ptr<Var> parseVar(bool is_volatile = false);
    std::unique_ptr<FuncCall> parseCall(std::optional<Ident> _ = std::nullopt);

    Ident parseIdent();

    Token forwardStream(uint8_t n = 1);
    Expression parseExpr(int min_bp = -1);
    Type* parseType();

    void parse();
    void performSema();
    void ignoreButExpect(const Token&);

    void toggleIsMainModule() { m_IsMainModule = !m_IsMainModule; }

    /// Calls `inserter` with the symbol name for each exported-symbol in the AST
    template <typename Inserter_t> requires std::invocable<Inserter_t, std::string>
    void insertExportedSymbolsInto(Inserter_t inserter) {
        for (const auto& node : AST) {
            if (node->is_exported) {
                inserter(node->getIdentInfo()->toString());
            }
        }
    }

    /// Decrements the unresolved-deps counter of dependents
    void decrementUnresolvedDeps();

    /// Buffers the reported errors, also sets certain context attributes automatically
    void reportError(const ErrCode code, ErrorContext ctx = {}) {
        ctx.src_man = &m_SrcMan;
        m_ErrorQueue.at(m_ParseStack.back()).emplace_back(code, ctx);
    }
};


struct Parser::NodeAttrHelper {
    /// Chief Node constructor
    NodeAttrHelper(Node* node, Parser& instance): node(node), instance(instance) {
        node->is_exported = instance.m_LastSymWasExported;
        node->location.from = instance.m_Stream.getStreamState();
        node->location.source = instance.m_FilePath;

        // setup error-report buffering
        if (!instance.m_ErrorQueue.contains(node))
            instance.m_ErrorQueue.insert({node, {}});
        instance.m_ParseStack.emplace_back(node);

        // other node specific attributes
        switch (node->getNodeType()) {
            case ND_VAR: {
                const auto nd = dynamic_cast<Var*>(node);
                nd->is_extern = instance.m_LastSymIsExtern;
                nd->extern_attributes = instance.m_ExternAttributes;
                break;
            } case ND_FUNC: {
                const auto nd = dynamic_cast<Function*>(node);
                nd->is_extern = instance.m_LastSymIsExtern;
                nd->extern_attributes = instance.m_ExternAttributes;
                break;
            } default:
                break;
        }
    }


    /// Resets the states of the Parser
    ~NodeAttrHelper() {
        instance.m_LastSymIsExtern = false;
        instance.m_LastSymWasExported = false;
        instance.m_ExternAttributes.clear();

        if (node)
            node->location.to = instance.m_Stream.getStreamState();

        // flush all the errors
        for (auto& error : instance.m_ErrorQueue.at(node)) {
            auto& context = std::get<1>(error);
            if (!context.location.has_value()) {
                if (node) context.location = node->location;
            } instance.m_ErrorCallback(std::get<0>(error), context);
        } instance.m_ErrorQueue.at(node).clear();
        instance.m_ParseStack.pop_back();

        // else type->location.to = instance.m_Stream.getStreamState();
    }

private:
    Node*   node = nullptr;
    Type*   type = nullptr;

    Parser& instance;
};
