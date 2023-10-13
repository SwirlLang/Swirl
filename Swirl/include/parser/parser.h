#include <array>
#include <list>

#include <memory>
#include <utility>
#include <tokenizer/Tokenizer.h>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H

enum NodeType {
    ND_INVALID,
    ND_EXPR,
    ND_INT,
    ND_FLOAT,
    ND_OP,
    ND_VAR,
    ND_STR,
    ND_CALL,
    ND_IDENT
};

struct Node {
    // Provides a common base class for all the nodes
    std::string value;

    virtual std::string getValue() const { return "INVALID"; }
    virtual NodeType getType() const { return ND_INVALID; };
};

struct Safeguard: Node {
    std::string getValue() const override { return "E"; }
};

struct Punc: Node { std::string getValue() const override { return "("; } };

struct Op: Node {
    std::string value;

    // the value will be 3 bytes at max so no need of a reference

    Op() {}
    explicit Op(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    [[nodiscard]] NodeType getType() const override {
        return ND_OP;
    }
};


struct Expression {
    std::vector<Op> operators{};
    std::vector<std::unique_ptr<Expression>> operands{};

    virtual NodeType getType() const {
        return ND_EXPR;
    }
};


struct Int: Node {
    std::string value = 0;

    explicit Int(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_INT;
    }
};


struct Float: Node {
    std::string value = 0;

    explicit Float(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_FLOAT;
    }
};


struct String: Node {
    std::string value;

    explicit String(std::string  val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_STR;
    }
};

struct Ident: Node {
    std::string value;

    explicit Ident(std::string  val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_IDENT;
    }
};

struct Var: Node {
    std::string var_ident;
    std::string var_type;
    Expression value;

    bool initialized = false;
    bool is_const    = false;

    std::string getValue() const override {
        return var_ident;
    }

    NodeType getType() const override {
        return ND_VAR;
    }
};

struct FuncCall: Node {
    std::vector<Expression> args;
    std::string ident;

    std::string getValue() const override { return ident; }
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
    void parseExpr(const std::string id = "");
    void parseLoop(TokenType);
//    void appendAST(Node&);
    inline void next(bool swsFlg = false, bool snsFlg = false);

    ~Parser();
};

#endif
