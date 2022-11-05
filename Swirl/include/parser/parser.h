#include <tokenizer/Tokenizer.h>
#include <map>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H

struct Node {
    std::string type;
    std::array<const char*, 2> value;
    std::string ident;
    bool append_chl = false;

    std::map<const char*, const char*> properties;
    std::vector<Node> chl;
    std::vector<Node> args;
};

struct AbstractSyntaxTree {
    std::vector<Node> chl;
};

class Parser {
public:
    TokenStream m_Stream;
    AbstractSyntaxTree* m_AST;
    bool m_AppendToScope = false;

    explicit Parser(TokenStream&);
    void parseCondition();
    void parseCall(const char*);
    void dispatch();
    void parseDecl();
    std::array<const char*, 2> next(std::array<const char*, 2>& _lvalue);
    ~Parser();
};

#endif
