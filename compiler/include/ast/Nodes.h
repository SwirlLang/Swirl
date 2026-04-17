#pragma once
#include <string_view>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>
#include <span>

#include "utils/utils.h"
#include "lexer/Tokens.h"
#include "parser/evaluation.h"
#include "symbols/IdentManager.h"
#include "utils/FileSystem.h"


#define SW_NODE_LIST \
    SW_NODE(ND_INVALID, Node) \
    SW_NODE(ND_EXPR, Expression) \
    SW_NODE(ND_INT, IntLit) \
    SW_NODE(ND_FLOAT, FloatLit) \
    SW_NODE(ND_OP, Op) \
    SW_NODE(ND_VAR, Var) \
    SW_NODE(ND_STR, StrLit) \
    SW_NODE(ND_CALL, FuncCall) \
    SW_NODE(ND_IDENT, Ident) \
    SW_NODE(ND_FUNC, Function) \
    SW_NODE(ND_RET, ReturnStatement) \
    SW_NODE(ND_COND, Condition) \
    SW_NODE(ND_WHILE, WhileLoop) \
    SW_NODE(ND_STRUCT, Struct) \
    SW_NODE(ND_IMPORT, ImportNode) \
    SW_NODE(ND_ARRAY, ArrayLit) \
    SW_NODE(ND_TYPE, TypeWrapper) \
    SW_NODE(ND_BOOL, BoolLit) \
    SW_NODE(ND_SCOPE, Scope) \
    SW_NODE(ND_BREAK, BreakStmt) \
    SW_NODE(ND_CONTINUE, ContinueStmt) \
    SW_NODE(ND_INTRINSIC, Intrinsic) \
    SW_NODE(ND_PROTOCOL, Protocol) \
    SW_NODE(ND_UNDEFINED, UndefinedValue) \
    SW_NODE(ND_ENUM, Enum)



#define SW_NODE(x, y) x,
enum NodeType {
    SW_NODE_LIST
};
#undef SW_NODE


struct Node;
struct Var;
struct Type;
struct Ident;
struct FunctionType;

class Parser;
class Namespace;
class IdentInfo;
class LLVMBackend;

using SwNode   = std::unique_ptr<Node>;
using NodesVec = std::vector<SwNode>;


struct SourceLocation {
    StreamState     from;
    StreamState     to;
    sw::FileHandle* source = nullptr;
};


// The common base class of all the nodes
struct Node {
    SourceLocation location;
    NodeType kind;

    bool is_exported = false;

    Node(): kind(ND_INVALID) {}
    Node(const NodeType ty): kind(ty) {}

    virtual ~Node() = default;


    /// Returns a tag which identifies the node's kind
    [[nodiscard]] virtual NodeType getNodeType() const {
        return ND_INVALID;
    }

    /// Is the node allowed to appear in global context (E.g. Functions, Vars)?
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

    virtual std::string toString() const {
        throw std::runtime_error("Node::toString: not implemented!");
    }

    virtual void replaceType(std::string_view from, Type* to) {}

    virtual EvalResult evaluate(Parser&);
    virtual SwNode clone() {
        throw std::runtime_error("clone unimplemented!");
    }

    template <typename From>
    std::add_pointer_t<From> to() {
        return dynamic_cast<std::add_pointer_t<From>>(this);
    }
};

enum class ErrCode;
struct ErrorContext;

struct GenericParam final : Node {
    std::string name;

    std::string toString() const override {
        return name;
    }
};

struct GlobalNode : Node {
    bool is_extern   = false;
    std::string extern_attributes;

    std::vector<GenericParam> generic_params;

    explicit GlobalNode(const NodeType ty): Node(ty) {}

    [[nodiscard]]
    bool isGlobal() const override {
        return true;
    }

    virtual std::unique_ptr<Node> instantiate(Parser& instance,
        std::span<Type*>,
        std::function<void (ErrCode, ErrorContext)>)
    {
        throw std::runtime_error("Generic instantiation unimplemented!");
    }
};


struct Expression final : Node {
    SwNode expr;
    Type* expr_type = nullptr;

    Expression(): Node(ND_EXPR) {}

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

    std::string toString() const override {
        return '(' + expr->toString() + ')';
    }

    void replaceType(const std::string_view from, Type* to) override {
        assert(expr != nullptr);
        expr->replaceType(from, to);
    }

    SwNode clone() override {
        auto ret = std::make_unique<Expression>();
        ret->expr = expr->clone();
        ret->expr_type = expr_type;
        return ret;
    }

    EvalResult     evaluate(Parser&) override;
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

    Op(): Node(ND_OP) {}

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

    std::string toString() const override {
        switch (arity) {
            case 1:
                return value + operands.at(0)->toString();
            case 2:
                return operands.at(0)->toString() + value + operands.at(1)->toString();
            default: throw std::runtime_error("Op::toString: invalid arity");;
        }
    }

    SwNode clone() override {
        auto ret = std::make_unique<Op>();
        ret->value = value;
        ret->arity = arity;

        for (auto& operand : operands) {
            ret->operands.push_back(operand->clone());
        }

        ret->is_mutable = is_mutable;
        ret->op_type = op_type;
        ret->common_type = common_type;
        return ret;
    }

    static int getLBPFor(OpTag_t op);
    static int getRBPFor(OpTag_t op);
    static int getPBPFor(OpTag_t op);

    void replaceType(std::string_view from, Type* to) override;

    EvalResult     evaluate(Parser& instance) override;
};


struct ReturnStatement final : Node {
    Expression value;
    FunctionType* parent_fn_type = nullptr;  // holds the function-signature of the parent

    ReturnStatement(): Node(ND_RET) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_RET;
    }



    std::string toString() const override {
        return "return " + value.toString();
    }

    SwNode clone() override {
        auto ret = std::make_unique<ReturnStatement>();
        ret->value = std::move(*dynamic_cast<Expression*>(value.clone().get()));
        ret->parent_fn_type = parent_fn_type;
        return ret;
    }

    void replaceType(const std::string_view from, Type* to) override {
        value.replaceType(from, to);
    }


};


struct UndefinedValue final : Node {
           UndefinedValue() : Node(ND_UNDEFINED) {}
};

struct IntLit final : Node {
    std::string value;

    explicit IntLit(std::string val): Node(ND_INT), value(std::move(val)) {}
    explicit IntLit(std::size_t val): Node(ND_INT), value(std::to_string(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_INT;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }

    std::string toString() const override { return value; }

    SwNode clone() override {
        return std::make_unique<IntLit>(value);
    }

    EvalResult   evaluate(Parser &) override;

};

struct FloatLit final : Node {
    std::string value;

    explicit FloatLit(std::string val):  Node(ND_FLOAT), value(std::move(val)) {}
    explicit FloatLit(const double val): Node(ND_FLOAT), value(std::to_string(val)) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FLOAT;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }

    std::string toString() const override { return value; }
    SwNode clone() override {
        return std::make_unique<FloatLit>(value);
    }

    EvalResult   evaluate(Parser &) override;
};


struct BoolLit final : Node {
    bool value;
    explicit BoolLit(const bool is_true) : Node(ND_BOOL), value(is_true) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_BOOL;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }

    std::string toString() const override { return value ? "true" : "false"; }
    SwNode clone() override {
        return std::make_unique<BoolLit>(value);
    }

    EvalResult   evaluate(Parser &) override;

};


struct StrLit final : Node {
    std::string value;

    explicit StrLit(std::string  val): Node(ND_STR), value(std::move(val)) {}

    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_STR;
    }

    [[nodiscard]]
    bool isLiteral() const override {
        return true;
    }

    std::string toString() const override { return value; }
    SwNode clone() override {
        return std::make_unique<StrLit>(value);
    }

    EvalResult   evaluate(Parser&) override;


};

struct TypeWrapper;
struct Ident final : Node {
private:
    struct ImplDeleter {
        void operator()(const TypeWrapper* ptr) const;
    };


    struct Qualifier {
        std::string name;
        std::vector<TypeWrapper*> generic_args;

        ~Qualifier();
    };

public:
    IdentInfo* value = nullptr;
    std::vector<Qualifier> full_qualification;

    Ident(): Node(ND_IDENT) {}

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

    std::string toString() const override {
        std::string ret;
        for (auto& id : full_qualification) {
            ret += id.name + "::";
        } ret.pop_back();
          ret.pop_back();
        return ret;
    }

    void replaceType(std::string_view from, Type* to) override;

    EvalResult     evaluate(Parser&) override;
};


/// Encodes all the necessary information needed to get a `Type*`.
struct TypeWrapper final : Node {
    enum Modifiers { Reference, Pointer };
    Type* type = nullptr;

    bool   is_mutable    = false;
    bool   is_slice      = false;

    Ident  type_id{};

    std::vector<Modifiers> modifiers{};
    std::size_t array_size = 0;            // 0 indicates that the type isn't an array
    std::unique_ptr<TypeWrapper> of_type{};  // set in the case of wrapper types (refs, ptr, arrays, slices)


    TypeWrapper(): Node(ND_TYPE) {}
    explicit TypeWrapper(Type* ty): Node(ND_TYPE), type(ty) {}

    [[nodiscard]]
    Type* getSwType() override {
        return type;
    }

    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_TYPE;
    }

    void replaceType(const std::string_view from, Type* to) override {
        if (!type_id.full_qualification.empty()) {
            if (type_id.full_qualification.front().name == from) {
                type = to;
            }
        } if (of_type) {
            of_type->replaceType(from, to);
        }
    }


    std::string toString() const override;

};

struct Var final : GlobalNode {
    IdentInfo* var_ident = nullptr;
    std::optional<TypeWrapper> var_type = std::nullopt;

    Expression value;

    bool initialized = false;
    bool is_const    = false;
    bool is_volatile = false;
    bool is_comptime = false;
    bool is_param    = false;
    bool is_instance_param = false;   // for the special case of `&self` in methods

    Var(): GlobalNode(ND_VAR) {}

    [[nodiscard]] NodeType getNodeType()  const override { return ND_VAR; }

    IdentInfo* getIdentInfo() override {
        return var_ident;
    }

    void replaceType(const std::string_view from, Type* to) override {
        var_type->replaceType(from, to);
        value.replaceType(from, to);
    }


    std::string toString() const override {
        std::string ret;

        if (!is_param) {
            if (is_comptime)
                ret += "comptime ";
            else if (is_const)
                ret += "let ";
            else ret += "var ";
        }

        ret += std::format("{}: {} ", var_ident->toString(), var_type->toString());
        if (initialized) {
            ret += std::format("= {};\n", value.toString());
        } else ret += ";\n";
        return ret;
    }

    [[nodiscard]] SwNode& getExprValue() override { return value.expr; }
};

struct Scope final : Node {
    std::vector<SwNode> children;

    Scope(): Node(ND_SCOPE) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_SCOPE;
    }

    void replaceType(const std::string_view from, Type* to) override {
        for (const auto& child : children) {
            child->replaceType(from, to);
        }
    }


    std::string toString() const override {
        std::string ret = "{\n";
        for (const auto& child : children) {
            ret += child->toString();
        } ret += "}\n";
        return ret;
    }
};


struct Function final : GlobalNode {
    IdentInfo* ident = nullptr;

    Function(): GlobalNode(ND_FUNC) {}

    std::vector<Var> params;
    std::vector<std::unique_ptr<Node>> children;

    TypeWrapper return_type;

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FUNC;
    }

    void replaceType(const std::string_view from, Type* to) override {
        for (auto& param : params) {
            param.replaceType(from, to);
        }

        return_type.replaceType(from, to);
        for (const auto& child : children) {
            child->replaceType(from, to);
        }
    }

    std::string toString() const override {
        std::string ret = std::format("fn {}", ident->toString());

        if (!generic_params.empty()) {
            ret += "<";
            for (auto& param : generic_params) {
                ret += param.toString();
            } ret += ">(";
        }

        for (auto& param : params) {
            ret += param.toString();
            ret += ", ";
        }

        if (ret.ends_with(", ")) {
            ret.pop_back();
            ret.pop_back();
        }

        ret += std::format("): {} {{", return_type.toString());

        for (auto& child : children) {
            ret += child->toString();
        } ret += "}\n";

        return ret;
    }

    std::unique_ptr<Node> instantiate(Parser&, std::span<Type*>, std::function<void (ErrCode, ErrorContext)>) override;
};


struct FuncCall final : Node {
    Ident ident;
    Type* signature = nullptr;  // supposed to hold the signature of the callee

    std::vector<Expression> args;
    std::vector<TypeWrapper> generic_args;

    FuncCall(): Node(ND_CALL) {}

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

    void replaceType(const std::string_view from, Type* to) override {
        for (auto& ty : generic_args) {
            ty.replaceType(from, to);
        }

        for (auto& val : args) {
            val.replaceType(from, to);
        }
    }

    std::string toString() const override {
        std::string ret = ident.toString();

        if (!generic_args.empty()) {
            ret.append("::<");
            for (auto& ty : generic_args) {
                ret += ty.toString() + ", ";
            }

            if (ret.ends_with(", ")) {
                ret.pop_back();
                ret.pop_back();
            }
        }

        ret += ">(";
        for (auto& val : args) {
            ret += val.toString();
            ret += ", ";
        }

        if (ret.ends_with(", ")) {
            ret.pop_back();
            ret.pop_back();
        } ret += ");";
        return ret;
    }
};


struct Intrinsic final : Node {
    enum Kind { INVALID, SIZEOF, TYPEOF, MEMCPY, MEMSET, ADV_PTR };
    Kind intrinsic_type = INVALID;

    std::vector<Expression> args;
    Ident ident;

    Intrinsic(): Node(ND_INTRINSIC) {}

    void operator=(const std::unique_ptr<FuncCall>& call) {
        args  = std::move(call->args);
        ident = std::move(call->ident);

        static const std::unordered_map<std::string, Kind> tag_map = {
            {"sizeof", SIZEOF},
            {"typeof", TYPEOF},
            {"memset", MEMSET},
            {"memcpy", MEMCPY},
            {"advance_pointer", ADV_PTR}
        }; intrinsic_type = tag_map.at(ident.full_qualification.at(0).name);
    }



    [[nodiscard]] NodeType getNodeType() const override { return ND_INTRINSIC; }

    void replaceType(const std::string_view from, Type* to) override {
        for (auto& arg : args) {
            arg.replaceType(from, to);
        }
    }
};


struct ImportNode final : Node {
    struct ImportedSymbol_t {
        std::string actual_name;
        std::string assigned_alias{};
    };

    ImportNode(): Node(ND_IMPORT) {}

    bool is_wildcard = false;

    sw::FileHandle*               mod_handle = nullptr;
    std::string                   alias;
    std::vector<ImportedSymbol_t> imported_symbols;

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_IMPORT;
    }
};


struct ArrayLit final : Node {
    Type* type = nullptr;
    std::vector<Expression> elements;

    ArrayLit(): Node(ND_ARRAY) {}

    Type* getSwType() override {
        return type;
    }

    [[nodiscard]]
    bool isLiteral() const override {
        return true;
    }

    void replaceType(const std::string_view from, Type* to) override {
        for (auto& element : elements) {
            element.replaceType(from, to);
        }
    }
};


struct WhileLoop final : Node {
    Expression condition;
    std::vector<std::unique_ptr<Node>> children{};

    WhileLoop(): Node(ND_WHILE) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_WHILE;
    }

    void replaceType(const std::string_view from, Type* to) override {
        condition.replaceType(from, to);
        for (const auto& child : children) {
            child->replaceType(from, to);
        }
    }
};


struct BreakStmt final : Node {
    BreakStmt(): Node(ND_BREAK) {}


    [[nodiscard]] NodeType getNodeType() const override { return ND_BREAK;}
};

struct ContinueStmt final : Node {
    ContinueStmt(): Node(ND_CONTINUE) {}

    [[nodiscard]] NodeType getNodeType() const override { return ND_CONTINUE; }
};


struct Enum final : Node {
    Enum() : Node(ND_ENUM), ident(nullptr) {}

    IdentInfo* ident;
    std::optional<TypeWrapper> enum_type;
    std::unordered_map<std::string, int> entries;

    int counter = 0;
    void addEntry(const std::string_view name) {
        entries.emplace(name, counter++);
    }
};

struct Protocol final : GlobalNode {
    struct MethodSignature {
        std::string name;
        TypeWrapper return_type;
        std::vector<TypeWrapper> params;

        std::string toString() const {
            std::string ret =  "fn " + name + "(";
            for (auto& param : params) {
                ret += param.toString();
            } ret += "): " + return_type.toString();
            return ret;
        }

        bool operator==(const MethodSignature& other) const {
            return name == other.name && other.return_type.type == return_type.type &&
                std::equal(params.begin(), params.end(),
                    other.params.begin(), other.params.end(),
                    [](const TypeWrapper& a, const TypeWrapper& b) {
                        return a.type == b.type;
                    });
        }

        void replaceType(const std::string_view from, Type* to) {
            return_type.replaceType(from, to);
            for (auto& ty : params) {
                ty.replaceType(from, to);
            }
        }
    };

    struct MemberSignature {
        std::string  name;
        TypeWrapper  type{};

        std::string toString() const {
            return name + ": " + type.toString();
        }

        bool operator==(const MemberSignature& other) const {
            return name == other.name && other.type.type == type.type;
        }

        void replaceType(const std::string_view from, Type* to) {
            type.replaceType(from, to);
        }
    };

    std::string protocol_name;
    IdentInfo*  protocol_id = nullptr;

    std::vector<Ident> depended_protocols;
    std::vector<MemberSignature> members;
    std::vector<MethodSignature> methods;

    IdentInfo* getIdentInfo() override {
        return protocol_id;
    }

    void replaceType(const std::string_view from, Type* to) override {
        for (auto& member : members) {
            member.replaceType(from, to);
        }

        for (auto& method : methods) {
            method.replaceType(from, to);
        }
    }

    Protocol(): GlobalNode(ND_PROTOCOL) {}
    NodeType getNodeType() const override { return ND_PROTOCOL; }

    std::unique_ptr<Node> instantiate(
        Parser& instance, std::span<Type*>, std::function<void(ErrCode, ErrorContext)>) override;
};


template <>
struct std::hash<TypeWrapper> {
    std::size_t operator()(const TypeWrapper& t) const noexcept {
        return std::hash<Type*>{}(t.type);
    }
};

template <>
struct std::hash<Protocol::MemberSignature> {
    std::size_t operator()(const Protocol::MemberSignature& m) const noexcept {
        return combineHashes(std::hash<std::string>{}(m.name), std::hash<TypeWrapper>{}(m.type));
    }
};

template <>
struct std::hash<Protocol::MethodSignature> {
    std::size_t operator()(const Protocol::MethodSignature& m) const noexcept {
        std::size_t arg_hash = 0;
        for (auto& arg : m.params) {
            arg_hash = combineHashes(arg_hash, std::hash<TypeWrapper>{}(arg));
        }

        return combineHashes(
            std::hash<std::string>{}(m.name),
            std::hash<TypeWrapper>{}(m.return_type),
            arg_hash
            );
    }
};


struct Struct final : GlobalNode {
    IdentInfo* ident = nullptr;
    std::vector<std::unique_ptr<Node>> members;
    std::vector<Ident> protocols;

    Struct(): GlobalNode(ND_STRUCT) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_STRUCT;
    }

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    void replaceType(const std::string_view from, Type* to) override {
        for (auto& member : members) {
            member->replaceType(from, to);
        }
    }
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

    Condition(): Node(ND_COND) {}
    [[nodiscard]] NodeType getNodeType() const override {
        return ND_COND;
    }


    void replaceType(const std::string_view from, Type* to) override {
        bool_expr.replaceType(from, to);
        for (const auto& child : else_children) {
            child->replaceType(from, to);
        }

        for (const auto& child : if_children) {
            child->replaceType(from, to);
        }

        for (auto& child : elif_children) {
            std::get<0>(child).replaceType(from, to);
            for (auto& c : std::get<1>(child)) {
                c->replaceType(from, to);
            }
        }
    }
};