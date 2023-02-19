#include <tokenizer/Tokenizer.h>
#include <array>
#include <map>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H

struct Node {
    bool initialized = false;
    bool format      = false;

    TokenType type;
    std::string value;
    std::string ident;
    std::string ctx_type;
    std::string from;
    std::string impr;

    std::vector<Node> body;
    std::vector<Node> arg_nodes;
    std::vector<Node> template_args;

    std::unordered_map<std::string, std::size_t> loc;
};

struct AbstractSyntaxTree {
    std::vector<Node> chl;
};

class Parser {
    Token cur_rd_tok{};
public:
    TokenStream m_Stream;
    AbstractSyntaxTree* m_AST;
    bool m_AppendToScope = false;
    std::vector<std::string> registered_symbols{};

    explicit Parser(TokenStream&);

    void parseCondition(TokenType);
    void parseCall(const char*);
    void dispatch();
    void parseFunction();
    void parseDecl(const char*, const char*);
    void parseLoop(TokenType);
    void appendAST(Node&);
    inline void next();

    ~Parser();
};

#endif
