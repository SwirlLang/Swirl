#pragma once
#include <memory>

#include <parser/Nodes.h>
#include <backend/LLVMBackend.h>
#include <tokenizer/Tokenizer.h>
#include <tokens/Tokens.h>


class Parser {
    Token cur_rd_tok{};
    std::string m_LatestFuncRetType{};
    LLVMBackend m_LLVMInstance;

public:
    TokenStream m_Stream;
    bool m_AppendToScope = false;
    bool m_TokenToScope  = false;
    std::vector<std::string> registered_symbols{};

    explicit Parser(TokenStream);

    std::unique_ptr<Function> parseFunction();
    std::unique_ptr<Condition> parseCondition();
    std::unique_ptr<Node> parseCall();
    std::unique_ptr<Node> dispatch();
    std::unique_ptr<Var> parseVar(bool is_volatile = false);
    std::unique_ptr<WhileLoop> parseWhile();
    std::unique_ptr<ReturnStatement> parseRet();
    std::unique_ptr<Struct> parseStruct();

    Token forwardStream(uint8_t n = 1);
    // void parseExpr(std::variant<std::vector<Expression>*, Expression*>, bool isCall = false, std::optional<std::string> as_type = std::nullopt);
    Expression parseExpr(std::string_view type);

    void parse();
//    void appendAST(Node&);

    ~Parser();

    std::string parseType() {
        std::string ret = m_Stream.p_CurTk.value;
        forwardStream();

        while (m_Stream.p_CurTk.type == OP && m_Stream.p_CurTk.value == "*")
            ret += forwardStream().value;
        std::cout << "ParseType leaving at -> " << m_Stream.p_CurTk.value << ": " << to_string(m_Stream.p_CurTk.type) << std::endl;
        return ret;
    }
};
