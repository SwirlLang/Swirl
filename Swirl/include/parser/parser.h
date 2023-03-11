#include <tokenizer/Tokenizer.h>
#include <array>
#include <map>
#include <list>

#include <llvm/IR/Value.h>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H

class Node {
public:
    bool initialized = false;
    bool format      = false;

    TokenType type;
    std::string value;
    std::string ident;
    std::string ctx_type;
    std::string from;
    std::string impr;

    std::list<Node> body;
    std::list<Node> arg_nodes;
    std::list<Node> template_args;
    std::list<Node> expression;

    std::unordered_map<std::string, std::size_t> loc;

    virtual llvm::Value* codegen() {
        std::cout << "Unimplemented codegen virtual method in: " << typeid(*this).name() << std::endl;
        std::exit(1);
    };
};

class NdExpr : public Node {
public:
    Node          LHS;
    std::string   op;
    Node          RHS;

    llvm::Value *codegen() override;
};

class NdInt : public Node {
public:
    long long value = 0;

    llvm::Value *codegen() override;
};

class NdFloat : public Node {
public:
    float value = 0.0f;
    llvm::Value *codegen() override;

};

class NdDouble : public Node {
public:
    double value = 0.0;

    llvm::Value *codegen() override;
};

class NdAssignment : public Node {
public:
    Node value;
    std::string type;
    std::string ident;
    bool initialized = false;
};

class NdStringLiteral : public Node {
public:
    std::string value;

    llvm::Value *codegen() override;
};

class NdCall : public Node {
public:
    std::string callee;

    llvm::Value *codegen() override;
};

class NdIf : public Node {
public:
    NdExpr cond;
};

struct AbstractSyntaxTree {
    std::list<Node> chl;
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
    void parseCall();
    void dispatch();
    void parseFunction();
    void parseVars();
    void parseLoop(TokenType);
    void appendAST(Node&, bool isScope = false);
    void next(bool swsFlg = false, bool snsFlg = false );

    ~Parser();
};

#endif
