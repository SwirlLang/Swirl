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


inline std::size_t getNewModuleUID() {
    static std::size_t uid = 0;
    return uid++;
}

class Parser {
    TokenStream  m_Stream;
    Function*    m_LatestFuncNode = nullptr;
    bool         m_LastSymWasExported = false;
    bool         m_LastSymIsExtern = false;
    std::size_t  m_ModuleUID = getNewModuleUID();  // unique id for the parser-instance per-module
    
    std::filesystem::path m_FilePath;
    std::filesystem::path m_RelativeDir;
    
    struct ParsedIdent {
        std::string name;
        fs::path    mod_path;
    };

public:
    ErrorManager  ErrMan;
    SymbolManager SymbolTable{m_ModuleUID};

    std::vector<std::unique_ptr<Node>> AST;
    std::queue<std::function<void()>>  VerificationQueue;

    explicit Parser(const std::filesystem::path&);

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    Parser(Parser&& other) noexcept
    : m_Stream(std::move(other.m_Stream))
    , m_LatestFuncNode(other.m_LatestFuncNode)
    , m_LastSymWasExported(other.m_LastSymWasExported)
    , ErrMan(std::move(other.ErrMan))
    , SymbolTable(std::move(other.SymbolTable))
    , AST(std::move(other.AST))
    , VerificationQueue(std::move(other.VerificationQueue)) {}
    
    std::unique_ptr<Node> dispatch();
    std::unique_ptr<Function> parseFunction();
    std::unique_ptr<Condition> parseCondition();
    std::unique_ptr<WhileLoop> parseWhile();
    std::unique_ptr<ReturnStatement> parseRet();
    std::unique_ptr<Struct> parseStruct();

    std::unique_ptr<Var> parseVar(bool is_volatile = false);
    std::unique_ptr<FuncCall> parseCall(std::optional<ParsedIdent> _ = std::nullopt);

    ParsedIdent parseIdent();

    Token forwardStream(uint8_t n = 1);
    Expression parseExpr(std::optional<Type*> bound_type = std::nullopt);
    Type* parseType();

    void parse();
    void callBackend();
    void handleImports();
    void analyzeSemantics(std::vector<std::unique_ptr<Node>>&);

    void runPendingVerifications() {
        while (!VerificationQueue.empty()) {
            VerificationQueue.front()();
            VerificationQueue.pop();
        }
    }
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