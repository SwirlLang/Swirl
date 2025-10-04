#pragma once
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>
#include <string_view>

#include "utils/utils.h"
#include "lexer/Tokens.h"
#include "parser/evaluation.h"


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
    ND_TYPE,        // 17
    ND_BOOL,        // 18
    ND_SCOPE,       // 19
    ND_BREAK,       // 20
    ND_CONTINUE,    // 21
    ND_INTRINSIC,   // 22
};


struct Node;
struct Var;
struct Type;
struct Ident;
struct FunctionType;

class Parser;
class Namespace;
class IdentInfo;
class LLVMBackend;
class AnalysisContext;
namespace llvm { class Value; }

using SwNode   = std::unique_ptr<Node>;
using NodesVec = std::vector<SwNode>;


struct AnalysisResult {
    bool       is_erroneous       = false;
    Type*      deduced_type       = nullptr;
    Namespace* computed_namespace = nullptr;
};


struct SourceLocation {
    StreamState from;
    StreamState to;
    fs::path    source;
};


class CGValue {
public:
    CGValue(): m_LValue(nullptr), m_RValue(nullptr) {}
    CGValue(llvm::Value* lvalue, llvm::Value* rvalue): m_LValue(lvalue), m_RValue(rvalue) {}

    llvm::Value*   getLValue();
    llvm::Value*   getRValue(LLVMBackend&);

    static CGValue lValue(llvm::Value* lvalue);
    static CGValue rValue(llvm::Value* rvalue);

private:
    llvm::Value* m_LValue;
    llvm::Value* m_RValue;
};


// The common base class of all the nodes
struct Node {
    SourceLocation location;
    std::optional<AnalysisResult> analysis_cache;

    bool is_exported = false;

    Node() = default;
    virtual ~Node() = default;


    /// Returns a tag which identifies the node's kind
    [[nodiscard]] virtual NodeType getNodeType() const {
        return ND_INVALID;
    }

    /// Can the node allowed to appear in global context (E.g. Functions, Vars)?
    [[nodiscard]] virtual bool isGlobal() const {
        return false;
    }

    /// Is the node representing a literal?
    [[nodiscard]] virtual bool isLiteral() const {
        return false;
    }

    /// Fetches the `IdentInfo*` that a node is holding, if any
    virtual IdentInfo* getIdentInfo() {
        throw std::runtime_error("getIdentInfo called on Node instance");
    }

    /// Convenient method to fetch the wrapped-node from an expression
    virtual SwNode& getExprValue() {
        throw std::runtime_error("getExprValue called on Node instance");
    }

    /// Returns the wrapped-node if the node is a wrapper, returns the instance itself otherwise
    virtual Node* getWrappedNodeOrInstance() {
        return this;
    }

    [[nodiscard]] virtual Ident* getIdent() {
        return nullptr;
    }

    virtual Type* getSwType() {
        throw std::runtime_error("getSwType: unimplemented!");
    }

    virtual EvalResult evaluate(Parser&);
    virtual AnalysisResult analyzeSemantics(AnalysisContext&) {
        return {};
    }

    virtual CGValue llvmCodegen([[maybe_unused]] LLVMBackend &instance) {
        throw std::runtime_error("llvmCodegen called on Node instance");
    }
};


struct GlobalNode : Node {
    bool is_extern   = false;
    std::string extern_attributes;

    [[nodiscard]]
    bool isGlobal() const override {
        return true;
    }
};


struct Expression final : Node {
    SwNode expr;
    Type* expr_type = nullptr;

    Expression() = default;

    static Expression makeExpression(std::unique_ptr<Node>&& node) {
        Expression expr;
        expr.expr = std::move(node);
        return expr;
    }

    static Expression makeExpression(const EvalResult& e);
    static Expression makeExpression(Node* node) {
        return makeExpression(std::unique_ptr<Node>(node));
    }

    // set the type of sub-expression instances to `to`
    void setType(Type* to);

    IdentInfo* getIdentInfo() override {
        return expr->getIdentInfo();
    }

    [[nodiscard]] SwNode& getExprValue() override {
        return expr;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_EXPR;
    }

    Node* getWrappedNodeOrInstance() override {
        return expr->getWrappedNodeOrInstance();
    }

    bool operator==(const Expression& other) const {
        return this->expr == other.expr && this->expr_type == other.expr_type;
    }

    Type* getSwType() override {
        return expr_type;
    }

    EvalResult   evaluate(Parser&) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
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
    Type*  common_type = nullptr;  // the common type of its operands

    Op() = default;

    explicit Op(std::string_view str, int8_t arity);
    static OpTag_t getTagFor(std::string_view str, int arity);

    // set the type of sub-expression instances to `to`
    void setType(Type* to) const;

    [[nodiscard]]
    NodeType getNodeType() const override { return ND_OP; }
    Type* getSwType() override { return common_type; }

    [[nodiscard]] SwNode& getLHS() {
        assert(!operands.empty());
        return operands.at(0);
    }

    [[nodiscard]] SwNode& getRHS() {
        assert(operands.size() >= 2);
        return operands.at(1);
    }

    static int getLBPFor(OpTag_t op);
    static int getRBPFor(OpTag_t op);
    static int getPBPFor(OpTag_t op);

    CGValue llvmCodegen(LLVMBackend &instance) override;
    EvalResult   evaluate(Parser& instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct ReturnStatement final : Node {
    Expression value;
    FunctionType* parent_fn_type = nullptr;  // holds the function-signature of the parent

    CGValue llvmCodegen(LLVMBackend &instance) override;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_RET;
    }

    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct IntLit final : Node {
    std::string value;

    explicit IntLit(std::string val): value(std::move(val)) {}
    explicit IntLit(std::size_t val): value(std::to_string(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_INT;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }

    EvalResult   evaluate(Parser &) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};

struct FloatLit final : Node {
    std::string value;

    explicit FloatLit(std::string val): value(std::move(val)) {}
    explicit FloatLit(const double val): value(std::to_string(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FLOAT;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }

    EvalResult   evaluate(Parser &) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct BoolLit final : Node {
    bool value;
    explicit BoolLit(const bool is_true) : value(is_true) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_BOOL;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }

    EvalResult   evaluate(Parser &) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
};


struct StrLit final : Node {
    std::string value;

    explicit StrLit(std::string  val): value(std::move(val)) {}

    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_STR;
    }

    [[nodiscard]]
    bool isLiteral() const override {
        return true;
    }

    EvalResult   evaluate(Parser&) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
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

    [[nodiscard]] Ident* getIdent() override {
        return this;
    }

    EvalResult   evaluate(Parser&) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


/// A wrapper class for `Type*`', enables its treatment as a node.
struct TypeWrapper final : Node {
    enum Modifiers { Reference, Pointer };
    Type* type = nullptr;

    bool   is_mutable    = false;
    bool   is_slice      = false;
    Ident  type_id;

    std::vector<Modifiers> modifiers;
    std::size_t array_size = 0;            // 0 indicates that the type isn't an array
    std::unique_ptr<TypeWrapper> of_type;  // set in the case of wrapper types (refs, ptr, arrays, slices)


    TypeWrapper() = default;
    explicit TypeWrapper(Type* ty): type(ty) {}

    [[nodiscard]]
    Type* getSwType() override {
        return type;
    }

    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_TYPE;
    }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct Var final : GlobalNode {
    IdentInfo* var_ident = nullptr;
    TypeWrapper var_type;

    Expression value;
    Node* parent = nullptr;

    bool initialized = false;
    bool is_const    = false;
    bool is_volatile = false;
    bool is_comptime = false;
    bool is_param    = false;
    bool is_instance_param = false;   // for the special case of `&self` in methods

    Var() = default;

    [[nodiscard]] NodeType getNodeType()  const override { return ND_VAR; }

    IdentInfo* getIdentInfo() override {
        return var_ident;
    }

    [[nodiscard]] SwNode& getExprValue() override { return value.expr; }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct GenericParam final : Node {
    std::string name;
};


struct Scope final : Node {
    std::vector<SwNode> children;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_SCOPE;
    }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct Function final : GlobalNode {
    IdentInfo* ident = nullptr;

    std::vector<Var> params;
    std::vector<GenericParam> generic_params;
    std::vector<std::unique_ptr<Node>> children;

    TypeWrapper return_type;

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FUNC;
    }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct FuncCall : Node {
    std::vector<Expression> args;
    Ident ident;
    Type* signature = nullptr;  // supposed to hold the signature of the callee

    IdentInfo* getIdentInfo() override {
        return ident.getIdentInfo();
    }

    Type* getSwType() override {
        return signature;
    }

    [[nodiscard]] Ident* getIdent() override {
        return &ident;
    }

    [[nodiscard]] NodeType     getNodeType() const override { return ND_CALL; }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct Intrinsic final : FuncCall {
    enum Kind { SIZEOF, TYPEOF };
    Kind intrinsic_type;

    Intrinsic() = delete;
    explicit Intrinsic(FuncCall&& fc): FuncCall(std::move(fc)) {
        static std::unordered_map<std::string, Kind> tag_map = {
            {"sizeof", SIZEOF},
            {"typeof", TYPEOF}
        }; intrinsic_type = tag_map.at(ident.full_qualification.at(0));
    }

    [[nodiscard]] NodeType getNodeType() const override { return ND_INTRINSIC; }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct ImportNode final : Node {
    struct ImportedSymbol_t {
        std::string actual_name;
        std::string assigned_alias{};
    };

    bool is_wildcard = false;

    std::filesystem::path         mod_path;
    std::string                   alias;
    std::vector<ImportedSymbol_t> imported_symbols;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_IMPORT;
    }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct ArrayLit final : Node {
    Type* type = nullptr;
    std::vector<Expression> elements;

    Type* getSwType() override {
        return type;
    }

    [[nodiscard]]
    bool isLiteral() const override {
        return true;
    }

    AnalysisResult analyzeSemantics(AnalysisContext&) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
};


struct WhileLoop final : Node {
    Expression condition{};
    std::vector<std::unique_ptr<Node>> children{};

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_WHILE;
    }

    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};


struct BreakStmt final : Node {
    CGValue llvmCodegen(LLVMBackend &instance) override;
    [[nodiscard]] NodeType getNodeType() const override { return ND_BREAK;}
};

struct ContinueStmt final : Node {
    CGValue llvmCodegen(LLVMBackend &instance) override;
    [[nodiscard]] NodeType getNodeType() const override { return ND_CONTINUE; }
};


struct Struct final : GlobalNode {
    IdentInfo* ident = nullptr;
    std::vector<std::unique_ptr<Node>> members;


    [[nodiscard]] NodeType getNodeType() const override {
        return ND_STRUCT;
    }

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    AnalysisResult analyzeSemantics(AnalysisContext&) override;

    CGValue llvmCodegen(LLVMBackend &instance) override;
};


struct Condition final : Node {
    Expression bool_expr;
    bool       is_comptime = false;

    std::vector<std::unique_ptr<Node>> if_children{};
    std::vector<std::tuple<Expression, std::vector<std::unique_ptr<Node>>>> elif_children;
    std::vector<std::unique_ptr<Node>> else_children{};

    [[nodiscard]] SwNode& getExprValue() override {
        return bool_expr.expr;
    }


    [[nodiscard]] NodeType getNodeType() const override {
        return ND_COND;
    }


    CGValue llvmCodegen(LLVMBackend &instance) override;
    AnalysisResult analyzeSemantics(AnalysisContext&) override;
};