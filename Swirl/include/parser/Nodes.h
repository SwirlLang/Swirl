#pragma once
#include <utility>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <backend/LLVMBackend.h>

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
    ND_FUNC,        //  9
    ND_RET,         // 10
    ND_ASSIGN,      // 11
    ND_COND,        // 12
    ND_WHILE,       // 13
    ND_STRUCT,      // 14
};


struct Var;
class IdentInfo;


// A common base class for all the nodes
struct Node {
    std::string value;
    std::size_t scope_id{};

    [[nodiscard]]
    virtual NodeType getNodeType() const { return ND_INVALID; }

    virtual bool hasScopes() { return false; }

    virtual IdentInfo* getIdentInfo() { throw std::runtime_error("getIdentInfo called on Node instance"); }

    virtual const std::vector<std::unique_ptr<Node>>& getExprValue() { throw std::runtime_error("getExprValue called on Node instance"); }

    virtual llvm::Value* llvmCodegen(LLVMBackend& instance) { return nullptr; }

    virtual int8_t getArity() { throw std::runtime_error("getArity called on base Node instance "); }

    virtual void print() { throw std::runtime_error("debug called on base Node"); }

    virtual void setArity(int8_t val) { throw std::runtime_error("setArity called on base Node instance"); }

    virtual std::vector<std::unique_ptr<Node>>& getMutOperands() { throw std::runtime_error("getMutOperands called on base node"); }

    virtual ~Node() = default;
};



struct Expression : Node {
    std::vector<std::unique_ptr<Node>> expr;
    Type* expr_type{};

    Expression() = default;
    Expression(Expression&& other) noexcept {
        expr.reserve(other.expr.size());
        expr_type = other.expr_type;

        std::move(
                std::make_move_iterator(other.expr.begin()),
                std::make_move_iterator(other.expr.end()),
                std::back_inserter(expr));
    }

    Expression& operator=(Expression&& other) noexcept {
        expr.clear();
        expr.reserve(other.expr.size());
        expr_type = other.expr_type;
        std::move(
                std::make_move_iterator(other.expr.begin()),
                std::make_move_iterator(other.expr.end()),
                std::back_inserter(expr));
        return *this;
    }

    IdentInfo* getIdentInfo() override {
        return expr.at(0)->getIdentInfo();
    }

    const std::vector<std::unique_ptr<Node>>& getExprValue() override {
        return expr;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_EXPR;
    }

    bool operator==(const Expression& other) const {
        return this->expr.data() == other.expr.data() && this->expr_type == other.expr_type;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct Op final : Expression {
    std::string value;
    int8_t arity = 2;  // the no. of operands the operator requires
    std::vector<std::unique_ptr<Node>> operands;

    Op() = default;
    explicit Op(std::string val): value(std::move(val)) {}


    void setArity(const int8_t val) override {
        arity = val;
    }

    int8_t getArity() override {
        return arity;
    }

    std::vector<std::unique_ptr<Node>>& getMutOperands() override {
        return operands;
    }


    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_OP;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};


struct Assignment final : Node {
    Expression  l_value;
    std::string op;
    Expression r_value;

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_ASSIGN;
    }
};

struct ReturnStatement final : Node {
    Expression value;
    Type* parent_ret_type;

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_RET;
    }
};

struct IntLit final : Expression {
    std::string value;

    explicit IntLit(std::string val): value(std::move(val)) {}


    [[nodiscard]] NodeType getNodeType() const override {
        return ND_INT;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};

struct FloatLit final : Expression {
    std::string value;

    explicit FloatLit(std::string val): value(std::move(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FLOAT;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct StrLit final : Expression {
    std::string value;

    explicit StrLit(std::string  val): value(std::move(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_STR;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};

struct Ident final : Expression {
    IdentInfo* value;

    explicit Ident(IdentInfo*  val): value(val) {}

    IdentInfo* getIdentInfo() override {
        return value;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_IDENT;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct Var final : Node {
    IdentInfo* var_ident = nullptr;
    Type* var_type = nullptr;

    Expression value;
    Node* parent = nullptr;

    bool initialized = false;
    bool is_const    = false;
    bool is_volatile = false;

    Var() = default;

    [[nodiscard]] NodeType getNodeType()  const override { return ND_VAR; }

    IdentInfo* getIdentInfo() override {
        return var_ident;
    }

    std::vector<std::unique_ptr<Node>>& getExprValue() override { return value.expr; }
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct Function final : Node {
    IdentInfo* ident = nullptr;
    Type* ret_type = nullptr;

    std::vector<Var> params{};
    std::vector<std::unique_ptr<Node>> children{};

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FUNC;
    }

    bool hasScopes() override {
        return true;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct FuncCall final : Expression {
    std::vector<Expression> args;
    IdentInfo* ident = nullptr;
    std::string type = "void";

    Node* parent = nullptr;

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    [[nodiscard]] NodeType     getNodeType() const override { return ND_CALL; }
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct WhileLoop final : Node {
    Expression condition{};
    std::vector<std::unique_ptr<Node>> children{};

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_WHILE;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};


struct Struct final : Node {
    IdentInfo* ident = nullptr;
    std::vector<std::unique_ptr<Node>> members{};

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_STRUCT;
    }

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};


struct Condition final : Node {
    Expression bool_expr{};
    std::vector<std::unique_ptr<Node>> if_children{};
    std::vector<std::tuple<Expression, std::vector<std::unique_ptr<Node>>>> elif_children;
    std::vector<std::unique_ptr<Node>> else_children{};

    const std::vector<std::unique_ptr<Node>>& getExprValue() override {
        return bool_expr.expr;
    }

    bool hasScopes() override {
        return true;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_COND;
    }

    
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};
