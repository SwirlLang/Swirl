#include <tokenizer/Tokenizer.h>
#include <array>
#include <map>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H

struct Node {
    bool initialized = false;

    std::string type;
    std::string value;
    std::string ident;
    std::string ctx_type;
};

struct AbstractSyntaxTree {
    std::vector<Node> chl;
};

class Parser {
    std::array<const char*, 2> cur_rd_tok{};
public:
    TokenStream m_Stream;
    AbstractSyntaxTree* m_AST;
    bool m_AppendToScope = false;
    std::vector<std::string> registered_symbols{};
    std::vector<std::string> registered_types = {"int", "string", "bool", "float", "var"};

    explicit Parser(TokenStream&);

    void parseCondition(const char*);
    void parseCall(const char*);
    void dispatch();
    void parseFunction();
    void parseDecl(const char*, const char*);
    void parseLoop(const char*);

    ~Parser();
};

#endif
