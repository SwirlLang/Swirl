#include <tokenizer/Tokenizer.h>
#include <array>
#include <map>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H

struct Node {
    std::string type;
    std::string value;
    std::string ident;
    std::size_t scope_order;

    bool initialized = true;
    std::string condition;
    std::vector<Node> chl;
    std::vector<Node> args;
};


struct AbstractSyntaxTree {
    std::vector<Node> chl;
};

class Parser {
    std::size_t m_ScopeId = 0;
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
    void appendAST(Node&);
    void parseDecl(const char*, const char*);
    void parseLoop(const char*);

    Node& getNodeById(std::size_t);
    std::array<const char*, 2> next(std::array<const char*, 2>& _lvalue);

    ~Parser();
};

#endif
