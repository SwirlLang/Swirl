#include <list>
#include <memory>
#include <utility>
#include <stack>
#include <variant>

#include <tokenizer/Tokenizer.h>
#include <llvm/IR/Value.h>
#include <exception/ExceptionHandler.h>

#ifndef SWIRL_PARSER_H
#define SWIRL_PARSER_H

enum NodeType {
    ND_INVALID,     //  0
    ND_EXPR,        //  1
    ND_INT,         //  2
    ND_FLOAT,       //  3
    ND_OP,          //  4
    ND_VAR,         //  5
    ND_STR,         //  6
    ND_CALL,        //  7
    ND_IDENT,       //  8
    ND_FUNC         //  9
};


// A common base class for all the nodes
struct Node {
    struct Param {
        std::string var_ident;
        std::string var_type;
        std::size_t symt_index;

        bool initialized = false;
        bool is_const    = false;
    };

    std::string value;
    Node* parent = nullptr;

    virtual const std::vector<std::unique_ptr<Node>>& getExprValue() { throw std::runtime_error("getExprValue called on Node instance"); }
    virtual Param getParamInstance() { return Param{}; }
    virtual void setParent(Node* pr) { }
    virtual Node* getParent() const { return nullptr; }
    virtual std::string getValue() const { throw std::runtime_error("getValue called on base node"); };
    virtual NodeType getType() const { throw std::runtime_error("getType called on base node"); };
    virtual std::vector<Param> getParams() const { throw std::runtime_error("getParams called on base getParams"); };
    virtual llvm::Value* codegen() { throw std::runtime_error("unimplemented Node::codegen"); }
    virtual int8_t getArity() { throw std::runtime_error("getArity called on base Node instance "); }
    virtual ~Node() = default;
};


struct Op: Node {
    std::string value;
    int8_t arity = 2;  // the no. of operands the operator requires, binary by default

    Op() = default;
    explicit Op(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    int8_t getArity() override { return arity; }
    [[nodiscard]] NodeType getType() const override {
        return ND_OP;
    }
};


struct Expression: Node {
    std::vector<std::unique_ptr<Node>> expr{};

    Expression() = default;
    Expression(Expression&& other) noexcept {
        expr.reserve(other.expr.size());
        std::move(
                std::make_move_iterator(other.expr.begin()),
                std::make_move_iterator(other.expr.end()),
                std::back_inserter(expr)
                );

        for (const auto& nd : expr) {
//            std::cout << "moving : " << nd->getValue() << std::endl;
        }
    }

    const std::vector<std::unique_ptr<Node>>& getExprValue() override { return expr; }

    std::string getValue() const override {
        throw std::runtime_error("getValue called on expression");
    }

    NodeType getType() const override {
        return ND_EXPR;
    }

    llvm::Value* codegen() override;
};


struct IntLit: Node {
    std::string value;

    explicit IntLit(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_INT;
    }

    llvm::Value *codegen() override;
};


struct FloatLit : Node {
    std::string value = 0;

    explicit FloatLit(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_FLOAT;
    }

    llvm::Value* codegen() override;
};


struct StrLit: Node {
    std::string value;

    explicit StrLit(std::string  val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_STR;
    }

    llvm::Value *codegen() override;
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

    llvm::Value* codegen() override;
};

struct Var: Node {
    std::string var_ident;
    std::string var_type;
    Expression value;
    Node* parent = nullptr;

    bool initialized = false;
    bool is_const    = false;

    Var() {};

    std::string getValue() const override { return var_ident; }
    NodeType getType() const override { return ND_VAR; }
    Node* getParent() const override { return parent; }
    void setParent(Node* pr) override { parent = pr; }

    std::vector<std::unique_ptr<Node>>& getExprValue() override { return value.expr; }
    llvm::Value* codegen() override;
};

struct Function: Node {
    std::string ident;
    std::string ret_type = "none";
    std::vector<Param> params{};
    std::vector<std::unique_ptr<Node>> children{};

    NodeType getType() const override {
        return ND_FUNC;
    }

    std::string getValue() const override {
        return ident;
    }

    std::vector<Param> getParams() const override {
        return params;
    }

    llvm::Value* codegen() override;
};

struct FuncCall: Node {
    std::vector<Expression> args;
    std::string ident;
    std::string type = "void";
    std::string getValue() const override { return ident; }
    Node* parent = nullptr;

    NodeType getType() const override { return ND_CALL; }
    void setParent(Node* pr) override { parent = pr; }
    Node* getParent() const override { return parent; }

    llvm::Value* codegen() override;
};

struct Condition: Node {
    Expression bool_expr{};

    const std::vector<std::unique_ptr<Node>>& getExprValue() override {
        return bool_expr.expr;
    }

    llvm::Value* codegen() override;
};


class Parser {
    Token cur_rd_tok{};
    ExceptionHandler m_ExceptionHandler{};

public:
    TokenStream m_Stream;
//    AbstractSyntaxTree* m_AST;
    bool m_AppendToScope = false;
    std::vector<std::string> registered_symbols{};

    explicit Parser(TokenStream&);

    std::unique_ptr<Function> parseFunction();
    void parseCondition();
    std::unique_ptr<Node> parseCall();
    std::unique_ptr<Node> dispatch();
    std::unique_ptr<Var> parseVar();
    void forwardStream(uint8_t n);
    void parseExpr(std::variant<std::vector<Expression>*, Expression*>, bool isCall = false);
    void parseLoop(TokenType);
    void parse();
//    void appendAST(Node&);
    inline void next(bool swsFlg = false, bool snsFlg = false);

    ~Parser();
};

#endif
