#pragma once
#include <memory>
#include <queue>

#include <parser/Nodes.h>
#include <types/SwTypes.h>
#include <tokenizer/TokenStream.h>
#include <tokenizer/Tokens.h>
#include <managers/ErrorManager.h>
#include <managers/SymbolManager.h>


class Parser {
    TokenStream  m_Stream;
    Function*    m_LatestFuncNode = nullptr;

public:
    ErrorManager  ErrMan;
    SymbolManager SymbolTable;

    std::vector<std::unique_ptr<Node>> AST;
    std::queue<std::function<void()>>  VerificationQueue;

    explicit Parser(std::string);

    std::unique_ptr<Node> dispatch();
    std::unique_ptr<FuncCall> parseCall();
    std::unique_ptr<Function> parseFunction();
    std::unique_ptr<Condition> parseCondition();
    std::unique_ptr<WhileLoop> parseWhile();
    std::unique_ptr<ReturnStatement> parseRet();
    std::unique_ptr<Struct> parseStruct();
    std::unique_ptr<Var> parseVar(bool is_volatile = false);

    Token forwardStream(uint8_t n = 1);
    Expression parseExpr(std::optional<Type*> bound_type = std::nullopt);
    Type* parseType();

    void parse();
    void callBackend();

    void runPendingVerifications() {
        while (!VerificationQueue.empty()) {
            VerificationQueue.front()();
            VerificationQueue.pop();
        }
    }

    ~Parser();

};
