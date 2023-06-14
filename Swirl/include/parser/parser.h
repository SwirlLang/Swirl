#include <array>
#include <list>

#include <memory>
#include <tokenizer/Tokenizer.h>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H


enum NodeType {
    ND_INVALID,
    ND_EXPR,
    ND_GROUP,
    ND_INT,
    ND_FLOAT,
    ND_DOUBLE,
    ND_OP,
    ND_VAR,
    ND_STR,
    ND_CALL,
    ND_ARG,
    ND_IDENT
};


struct Node {
    // Provides a common base class for all the nodes
    std::string value;

    virtual NodeType getType() const { return ND_INVALID; };
};

struct Expression: Node {
    // The order in which the expression is supposed to be evaluated is pushed into this array
    // the reading begins from the right hand side.
    unsigned int max_precedence = 10;
    std::vector<Expression> evaluation_ord;

    virtual NodeType getType() const {
        return ND_EXPR;
    }
};

struct ExprGroup: Expression {
    // Just a struct to group non-complex expressions
    unsigned int precedence = 0;
    std::vector<Expression> group_members;

    NodeType getType() const override {
        return ND_GROUP;
    }
};

struct Int: Expression {
    long long value = 0;

    explicit Int(long long val): value(val) {}

    NodeType getType() const override {
        return ND_INT;
    }
};

struct Float: Expression {
    float value = 0;

    NodeType getType() const override {
        return ND_FLOAT;
    }
};

struct Double: Expression {
    double value = 0;

    explicit Double(double val): value(val) {}
    NodeType getType() const override {
        return ND_DOUBLE;
    }
};

struct Op: Expression {
    std::string value;

    // the value will be 3 bytes at max so no need of a reference
    Op(std::string val): value(val) {}

    NodeType getType() const override {
        return ND_OP;
    }
};

struct Stmt: Node {
    // Base class for all statements
};

struct String: Expression {
    std::string value;

    String(const std::string& val): value(val) {}

    NodeType getType() const override {
        return ND_STR;
    }
};

struct Ident: Expression {
    std::string value;

    Ident(const std::string& val): value(val) {}

    NodeType getType() const override {
        return ND_IDENT;
    }
};

struct Var: Stmt {
    std::string var_ident;
    std::string var_type;
    Expression value;

    bool initialized = false;
    bool is_const    = false;

    NodeType getType() const override {
        return ND_VAR;
    }
};

struct FuncCall: Expression {
    std::vector<Expression> args;
    std::string ident;

    NodeType getType() const override {
        return ND_CALL;
    }
};

class Parser {
    Token cur_rd_tok{};
public:
    TokenStream m_Stream;
//    AbstractSyntaxTree* m_AST;
    bool m_AppendToScope = false;
    std::vector<std::string> registered_symbols{};

    explicit Parser(TokenStream&);

    void parseCondition(TokenType);
    std::unique_ptr<FuncCall> parseCall();
    void dispatch();
    void parseFunction();
    void parseVar();
    void parseExpr(std::vector<Expression>*, const std::string id = "");
    void parseLoop(TokenType);
//    void appendAST(Node&);
    inline void next(bool swsFlg = false, bool snsFlg = false);

    ~Parser();
};

#endif
