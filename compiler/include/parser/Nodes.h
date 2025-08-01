#pragma once
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>
#include <string_view>

#include <utils/utils.h>
#include <lexer/Tokens.h>


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
    ND_IMPORT,      // 15
    ND_ARRAY,       // 16
};


struct Var;
struct Type;

class IdentInfo;
class LLVMBackend;
class AnalysisContext;
namespace llvm { class Value; }


struct AnalysisResult {
    bool is_erroneous = false;
    Type* deduced_type = nullptr;
};


// The common base class of all the nodes
struct Node {
    std::string value;
    std::size_t scope_id{};
    StreamState location;

    bool is_exported = false;

    [[nodiscard]]
    virtual NodeType getNodeType() const {
        return ND_INVALID;
    }

    virtual bool hasScopes() {
        return false;
    }

    virtual IdentInfo* getIdentInfo() {
        throw std::runtime_error("getIdentInfo called on Node instance");
    }

    virtual const std::vector<std::unique_ptr<Node>>& getExprValue() {
        throw std::runtime_error("getExprValue called on Node instance");
    }

    virtual llvm::Value* llvmCodegen(LLVMBackend& instance) {
        throw std::runtime_error("llvmCodegen called on Node instance");
    }

    virtual int8_t getArity() {
        throw std::runtime_error("getArity called on base Node instance ");
    }

    virtual void setArity(int8_t val) {
        throw std::runtime_error("setArity called on base Node instance");
    }

    virtual std::vector<std::unique_ptr<Node>>& getMutOperands() {
        throw std::runtime_error("getMutOperands called on base node");
    }

    virtual AnalysisResult analyzeSemantics(AnalysisContext&) {
        return {};
    }

    virtual Type* getSwType() {
        throw std::runtime_error("getTypeTag unimplemented!");
    }

    virtual ~Node() = default;
};


struct Expression final : Node {
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

    static Expression makeExpression(std::unique_ptr<Node>&& node) {
        Expression expr;
        expr.expr.push_back(std::move(node));
        return std::move(expr);
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
    std::string value;
    int8_t arity = 2;  // the no. of operands the operator requires
    std::vector<std::unique_ptr<Node>> operands;  // the operands

    // for the special case of the `&` operator, where a `mut` can appear right after it
    bool is_mutable = false;

    enum OpTag_t {
        BINARY_ADD,
        BINARY_SUB,

        MUL,
        DIV,
        MOD,

        UNARY_ADD,
        UNARY_SUB,

        LOGICAL_AND,
        LOGICAL_OR,
        LOGICAL_NOT,
        LOGICAL_NOTEQUAL,
        LOGICAL_EQUAL,

        GREATER_THAN,
        GREATER_THAN_OR_EQUAL,
        LESS_THAN,
        LESS_THAN_OR_EQUAL,

        DOT,
        INDEXING_OP,
        DEREFERENCE,
        ADDRESS_TAKING,
        CAST_OP,

        ASSIGNMENT,
        ADD_ASSIGN,
        MUL_ASSIGN,
        SUB_ASSIGN,
        DIV_ASSIGN,
        MOD_ASSIGN,

        INVALID
    };

    OpTag_t op_type = INVALID;
    Type*  inferred_type = nullptr;

    Op() = default;

    explicit Op(std::string_view str, int8_t arity);
    static OpTag_t getTagFor(std::string_view str, int arity);

    void setArity(const int8_t val) override {
        arity = val;
    }

    int8_t getArity() override {
        return arity;
    }


    std::vector<std::unique_ptr<Node>>& getMutOperands() override { return operands; }

    // set the type of sub-expression instances to `to`
    void setType(Type* to);

    [[nodiscard]]
    NodeType getNodeType() const override { return ND_OP; }
    Type* getSwType() override { return inferred_type; }

    static int getLBPFor(OpTag_t op);
    static int getRBPFor(OpTag_t op);
    static int getPBPFor(OpTag_t op);

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
    IdentInfo* value = nullptr;
    std::vector<std::string> full_qualification;

    Ident() = default;
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
    bool is_extern = false;  // whether it has been extern'd

    std::string extern_attributes;

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

    bool is_extern = false;
    std::string extern_attributes;

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

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct FuncCall final : Node {
    std::vector<Expression> args;
    Ident ident;
    Type* signature = nullptr;  // supposed to hold the signature of the callee

    // a flag, if set, indicates existence in another module to the backend
    std::filesystem::path module_path;

    Node* parent = nullptr;

    IdentInfo* getIdentInfo() override {
        return ident.getIdentInfo();
    }

    Type* getSwType() override {
        return signature;
    }

    [[nodiscard]] NodeType     getNodeType() const override { return ND_CALL; }
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct ImportNode final : Node {
    struct ImportedSymbol_t {
        std::string actual_name;
        std::string assigned_alias;
    };

    bool is_wildcard = false;

    std::filesystem::path         mod_path;
    std::string                   alias;
    std::vector<ImportedSymbol_t> imported_symbols;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_IMPORT;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct ArrayNode final : Node {
    Type* type = nullptr;
    std::vector<Expression> elements;

    Type* getSwType() override {
        return type;
    }

    AnalysisResult analyzeSemantics(AnalysisContext&) override;
    llvm::Value*   llvmCodegen(LLVMBackend& instance) override;
};


struct WhileLoop final : Node {
    Expression condition{};
    std::vector<std::unique_ptr<Node>> children{};

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_WHILE;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct Struct final : Node {
    IdentInfo* ident = nullptr;
    std::vector<std::unique_ptr<Node>> members;

    bool is_externed = false;
    std::string extern_attributes;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_STRUCT;
    }

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    AnalysisResult analyzeSemantics(AnalysisContext&) override;

    llvm::Value* llvmCodegen(LLVMBackend &instance) override;
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