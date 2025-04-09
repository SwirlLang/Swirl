#pragma once
#include <filesystem>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <utility>

#include <parser/Nodes.h>
#include <types/SwTypes.h>
#include <tokenizer/TokenStream.h>
#include <tokenizer/Tokens.h>
#include <managers/ErrorManager.h>
#include <managers/SymbolManager.h>


class Parser {
    TokenStream  m_Stream;

    // ---*--- Flags  ---*---
    Function*    m_LatestFuncNode = nullptr;
    bool         m_LastSymWasExported = false;
    bool         m_LastSymIsExtern = false;

    std::optional<Token> m_ReturnFakeToken = std::nullopt;
    // ---*--- ---*--- ---*---


    std::filesystem::path m_FilePath;
    std::filesystem::path m_RelativeDir;


public:
    ErrorManager  ErrMan;
    SymbolManager SymbolTable;

    std::vector<std::unique_ptr<Node>> AST;
    std::unordered_map<IdentInfo*, Node*> GlobalNodeJmpTable;  // maps global symbols to their nodes

    explicit Parser(const std::filesystem::path&);

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    Parser(Parser&& other) noexcept
    : m_Stream(std::move(other.m_Stream))
    , m_LatestFuncNode(other.m_LatestFuncNode)
    , m_LastSymWasExported(other.m_LastSymWasExported)
    , ErrMan(std::move(other.ErrMan))
    , SymbolTable(std::move(other.SymbolTable))
    , AST(std::move(other.AST)) {}
    
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
    Expression parseExpr(const std::optional<Token>& terminator = std::nullopt);
    Type* parseType();

    void parse();
    void callBackend();
    void analyzeSemantics(std::vector<std::unique_ptr<Node>>&);
};


class ModuleMap_t {
    std::unordered_map<std::filesystem::path, Parser> m_ModuleMap;
    std::shared_mutex m_Mutex;
    
public:
    Parser& get(const std::filesystem::path& m) {
        std::shared_lock guard(m_Mutex);
        return m_ModuleMap.at(m);
    }
    
    void insert(const std::filesystem::path& key, Parser parser) {
        std::lock_guard guard(m_Mutex);
        m_ModuleMap.emplace(key, std::move(parser));
    }

    bool contains(const std::filesystem::path& mod) {
        std::shared_lock guard(m_Mutex);
        return m_ModuleMap.contains(mod);
    } 
};