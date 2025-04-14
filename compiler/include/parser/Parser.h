#pragma once
#include <filesystem>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
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

    std::size_t  m_UnresolvedDeps{};

    std::filesystem::path m_FilePath;
    std::filesystem::path m_RelativeDir;

    std::unordered_set<Parser*> m_Dependents;     // the modules which depend on this module
    std::unordered_set<Parser*> m_Dependencies;  // the modules which this module depends on

    friend class Module;
    friend class ModuleMap_t;
    friend class LLVMBackend;

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
    void performSema();
    void callBackend();

    /// decrements the unresolved-deps counter of dependents
    void decrementUnresolvedDeps();
};


class ModuleMap_t {
    std::unordered_map<std::filesystem::path, std::unique_ptr<Parser>> m_ModuleMap;
    std::vector<Parser*> m_ZeroDepVec;  // holds modules with zero dependencies
    std::vector<Parser*> m_BackBuffer;  // this will be swapped with the above vector after flushing

    friend class Parser;

public:
    Parser& get(const std::filesystem::path& m) const {
        return *m_ModuleMap.at(m);
    }

    Parser* popZeroDepVec() {
        if (m_ZeroDepVec.empty())
            return nullptr;

        const auto ret = m_ZeroDepVec.back();
        m_ZeroDepVec.pop_back();

        for (Parser* dependent : ret->m_Dependents) {
            dependent->decrementUnresolvedDeps();
        }

        return ret;
    }

    void swapBuffers() {
        if (!m_BackBuffer.empty())
            std::swap(m_BackBuffer, m_ZeroDepVec);
    }

    void insert(const std::filesystem::path& key, Parser parser) {
        m_ModuleMap.emplace(key, std::make_unique<Parser>(std::move(parser)));
    }

    bool zeroVecIsEmpty() const {
        return m_ZeroDepVec.empty();
    }

    bool contains(const std::filesystem::path& mod) const {
        return m_ModuleMap.contains(mod);
    } 
};