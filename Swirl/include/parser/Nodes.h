#pragma once
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <tokenizer/Tokens.h>

class LLVMBackend;

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
struct Type;


struct AnalysisResult {
    bool is_erroneous = false;
    Type* deduced_type = nullptr;
};

class AnalysisContext;


// A common base class for all the nodes
struct Node {
    std::string value;
    std::size_t scope_id{};
    StreamState location;

    [[nodiscard]]
    virtual NodeType getNodeType() const { return ND_INVALID; }

    virtual bool hasScopes() { return false; }

    virtual IdentInfo* getIdentInfo() { throw std::runtime_error("getIdentInfo called on Node instance"); }

    virtual const std::vector<std::unique_ptr<Node>>& getExprValue() { throw std::runtime_error("getExprValue called on Node instance"); }

    virtual llvm::Value* llvmCodegen(LLVMBackend& instance) { throw std::runtime_error("llvmCodegen called on Node instance"); }

    virtual int8_t getArity() { throw std::runtime_error("getArity called on base Node instance "); }

    virtual void print() { throw std::runtime_error("debug called on base Node"); }

    virtual void setArity(int8_t val) { throw std::runtime_error("setArity called on base Node instance"); }

    virtual std::vector<std::unique_ptr<Node>>& getMutOperands() { throw std::runtime_error("getMutOperands called on base node"); }
    
    virtual AnalysisResult analyzeSemantics(AnalysisContext&) { return {}; }

    virtual Type* getSwType() { throw std::runtime_error("getSwType unimplemented!"); }

    virtual ~Node() = default;
};


struct Expression : Node {
    std::vector<std::unique_ptr<Node>> expr;
    Type* expr_type = nullptr;

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

    // set the type of sub-expression instances to `to`
    void setType(Type* to);

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

    Type* getSwType() override {
        return expr_type;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;

    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct Op final : Node {
    std::string value;  // the kind of operator (e.g. '+', '-')
    int8_t arity = 2;  // the no. of operands the operator requires
    std::vector<std::unique_ptr<Node>> operands;  // the operands

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

    // set the type of sub-expression instances to `to`
    void setType(Type* to);

    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_OP;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct Assignment final : Node {
    Expression  l_value;
    std::string op;
    Expression r_value;

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_ASSIGN;
    }
    
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct ReturnStatement final : Node {
    Expression value;
    Type* parent_ret_type;

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_RET;
    }

    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct IntLit final : Node {
    std::string value;

    explicit IntLit(std::string val): value(std::move(val)) {}


    [[nodiscard]] NodeType getNodeType() const override {
        return ND_INT;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct FloatLit final : Node {
    std::string value;

    explicit FloatLit(std::string val): value(std::move(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FLOAT;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct StrLit final : Node {
    std::string value;

    explicit StrLit(std::string  val): value(std::move(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_STR;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct Ident final : Node {
    IdentInfo* value;

    explicit Ident(IdentInfo* val): value(val) {}

    IdentInfo* getIdentInfo() override {
        return value;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_IDENT;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct Var final : Node {
    IdentInfo* var_ident = nullptr;
    Type* var_type = nullptr;

    Expression value;
    Node* parent = nullptr;

    bool initialized = false;
    bool is_const    = false;
    bool is_volatile = false;
    bool is_exported = false;
    bool is_extern = false;  // whether it has been extern'd

    Var() = default;

    [[nodiscard]] NodeType getNodeType()  const override { return ND_VAR; }

    IdentInfo* getIdentInfo() override {
        return var_ident;
    }

    std::vector<std::unique_ptr<Node>>& getExprValue() override { return value.expr; }
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct Function final : Node {
    IdentInfo* ident = nullptr;
    Type** reg_ret_type = nullptr;

    bool is_exported = false;
    bool is_extern = false;  // whether it has been extern'd

    std::vector<Var> params;
    std::vector<std::unique_ptr<Node>> children;

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FUNC;
    }

    bool hasScopes() override {
        return true;
    }

    void updateRetTypeTo(Type* to) const {
        *reg_ret_type = to;
        // ret_type = to;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct FuncCall final : Expression {
    std::vector<Expression> args;
    IdentInfo* ident = nullptr;
    Type*      signature = nullptr;  // supposed to hold the signature of the callee

    // a flag, if set, indicates existence in another module to the backend
    std::filesystem::path module_path;

    Node* parent = nullptr;

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    Type* getSwType() override {
        return signature;
    }

    [[nodiscard]] NodeType     getNodeType() const override { return ND_CALL; }
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct WhileLoop final : Node {
    Expression condition{};
    std::vector<std::unique_ptr<Node>> children{};

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_WHILE;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
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

    AnalysisResult analyzeSemantics(AnalysisContext&) override;
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
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};
