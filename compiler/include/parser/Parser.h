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
#include "managers/ErrorManager.h"
#include "symbols/SymbolManager.h"

#include "ExpressionParser.h"


class Parser;
class ModuleManager;
class AnalysisContext;
using ErrorCallback_t = std::function<void (ErrCode, ErrorContext)>;


class Parser {
    TokenStream    m_Stream;
    SourceManager  m_SrcMan;
    ModuleManager& m_ModuleMap;

    ErrorCallback_t m_ErrorCallback;  // the callback for reporting an error
    ExpressionParser m_ExpressionParser{*this};

    // ---*--- Flags  ---*---
    Function*    m_LatestFuncNode = nullptr;
    bool         m_LastSymWasExported = false;
    bool         m_LastSymIsExtern = false;
    bool         m_IsMainModule    = false;  // is the module the parser represents the main one?

    std::string m_ExternAttributes;
    std::optional<Token> m_ReturnFakeToken = std::nullopt;
    // ---*--- ---*--- ---*---

    std::size_t  m_UnresolvedDeps{};

    std::filesystem::path m_FilePath;
    std::filesystem::path m_RelativeDir;

    std::unordered_set<Parser*> m_Dependents;     // the modules which depend on this module
    std::unordered_set<Parser*> m_Dependencies;  // the modules which this module depends on

    struct Bracket_t { char val; StreamState location; };
    std::vector<Bracket_t> m_BracketTracker;

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
    , m_RelativeDir(std::move(other.m_RelativeDir))
    , m_Dependents(std::move(other.m_Dependents))
    , m_Dependencies(std::move(other.m_Dependencies))
    , SymbolTable(std::move(other.SymbolTable))
    , AST(std::move(other.AST))
    , NodeJmpTable(std::move(other.NodeJmpTable))
    {
        m_RelativeDir = m_FilePath.parent_path();
    }


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

    /// calls `inserter` with the symbol name for each exported-symbol in the AST
    template <typename Inserter_t> requires std::invocable<Inserter_t, std::string>
    void insertExportedSymbolsInto(Inserter_t inserter) {
        for (const auto& node : AST) {
            if (node->is_exported) {
                inserter(node->getIdentInfo()->toString());
            }
        }
    }

    /// decrements the unresolved-deps counter of dependents
    void decrementUnresolvedDeps();

    void reportError(const ErrCode code, ErrorContext ctx = {}) {
        ctx.src_man = &m_SrcMan;
        if (!ctx.location) {
            ctx.location = m_Stream.getStreamState();
        } m_ErrorCallback(code, ctx);
    }
};
