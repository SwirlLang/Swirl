#pragma once
#include <ranges>
#include <string_view>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <variant>
#include <span>

#include "utils/utils.h"
#include "lexer/Tokens.h"
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
    SW_NODE(ND_ENUM, Enum) \
    SW_NODE(ND_GEN_ARG, GenericArg) \
    SW_NODE(ND_GEN_ARG_LIST, GenericArgList)


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


struct SourceLocation {
    StreamState     from;
    StreamState     to;
    sw::FileHandle* source = nullptr;
};


// The common base class of all the nodes
struct Node {
    SourceLocation location;
    NodeType kind = ND_INVALID;

    Node() = default;
    explicit Node(const NodeType ty)
        : kind(ty) {}

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
    virtual Node* getExprValue() {
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

    template <typename From>
    std::add_pointer_t<From> to() {
        return dynamic_cast<std::add_pointer_t<From>>(this);
    }
};


struct GenericParam final : Node {
    std::string_view name;
    IdentInfo*       id;

    explicit
    GenericParam() : Node(ND_INVALID), id(nullptr) {}
};


/// Nodes which can appear in the Global Scope inherit from `GlobalNode`.
struct GlobalNode : Node {
    bool is_extern   = false;
    bool is_exported = false;

    std::string_view name;

    std::string_view extern_attributes;
    std::span<GenericParam*> generic_params;

    explicit GlobalNode(const NodeType ty)
        : Node(ty) {}

    [[nodiscard]]
    bool isGlobal() const override {
        return true;
    }
};


struct Expression final : Node {
    bool is_comptime = false;

    Node* expr = nullptr;
    Type* expr_type = nullptr;

    explicit Expression(): Node(ND_EXPR) {}

    static Expression makeExpression(Node* node) {
        Expression expr;
        expr.expr = node;
        return expr;
    }


    // set the type of sub-expression instances to `to`
    void setType(Type* to);

    IdentInfo* getIdentInfo() override {
        return expr->getIdentInfo();
    }

    [[nodiscard]] Node* getExprValue() override {
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
};


struct Op final : Node {
    int8_t arity = 2;  // the no. of operands the operator requires

    Token::TokenValue tokenid{};
    std::span<Node*> operands;  // the operands

    // for the special case of the `&` operator, where a `mut` can appear right after it
    bool is_mutable = false;

    enum OpTag_t {
        BINARY_ADD,
        BINARY_SUB,

        MUL,
        DIV,
        MOD,
        EXP,
        EXP_ASSIGN,

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

        BITWISE_OR,
        BITWISE_AND,
        BITWISE_XOR,
        BITWISE_NOT,
        BITWISE_LSHIFT,
        BITWISE_RSHIFT,

        BITWISE_OR_ASSIGN,
        BITWISE_AND_ASSIGN,
        BITWISE_XOR_ASSIGN,
        BITWISE_LSHIFT_ASSIGN,
        BITWISE_RSHIFT_ASSIGN,

        INVALID
    };

    OpTag_t op_type = INVALID;
    Type*  common_type = nullptr;  // the common type of its operands

    explicit Op()
        : Node(ND_OP) {}

    explicit Op(Token::TokenValue tokenid, int8_t arity);
    static OpTag_t getTagFor(Token::TokenValue tokenid, int arity);

    // set the type of sub-expression instances to `to`
    void setType(Type* to) const;

    [[nodiscard]]
    NodeType getNodeType() const override { return ND_OP; }
    Type* getSwType() override { return common_type; }

    [[nodiscard]] Node* getLHS() const {
        assert(!operands.empty());
        return operands.at(0);
    }

    [[nodiscard]] Node* getRHS() const {
        assert(operands.size() >= 2);
        return operands.at(1);
    }

    static int getLBPFor(OpTag_t op);
    static int getRBPFor(OpTag_t op);
    static int getPBPFor(OpTag_t op);
};


struct ReturnStatement final : Node {
    Expression* value = nullptr;
    FunctionType* parent_fn_type = nullptr;  // holds the function-signature of the parent

    explicit
    ReturnStatement()
        : Node(ND_RET) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_RET;
    }
};


struct UndefinedValue final : Node {
    explicit UndefinedValue() : Node(ND_UNDEFINED) {}
};


struct IntLit final : Node {
    std::string_view value;

    explicit IntLit(const std::string_view val)
        : Node(ND_INT), value(val) {}


    [[nodiscard]] NodeType getNodeType() const override {
        return ND_INT;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }
};


struct FloatLit final : Node {
    std::string_view value;

    explicit FloatLit(const std::string_view val)
        : Node(ND_FLOAT), value(val) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FLOAT;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }
};


struct BoolLit final : Node {
    bool value;

    explicit BoolLit(const bool is_true)
        : Node(ND_BOOL), value(is_true) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_BOOL;
    }

    [[nodiscard]] bool isLiteral() const override {
        return true;
    }
};


struct StrLit final : Node {
    std::string_view value;

    explicit StrLit(const std::string_view val)
        : Node(ND_STR), value(val) {}

    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_STR;
    }

    [[nodiscard]]
    bool isLiteral() const override {
        return true;
    }
};


struct GenericArg;
struct GenericArgList final : Node {
    std::span<GenericArg*> generic_args;
    GenericArgList()
        : Node(ND_GEN_ARG_LIST) {}

    [[nodiscard]] auto front() const   { return generic_args.front(); }
    [[nodiscard]] auto begin() const   { return generic_args.begin(); }
    [[nodiscard]] auto end()   const   { return generic_args.end(); }

    [[nodiscard]] auto size()    const { return generic_args.size(); }
    [[nodiscard]] bool empty()   const { return generic_args.empty(); }

    [[nodiscard]] auto at(const std::size_t i) const { return generic_args.at(i); }
};


struct Ident final : Node {
    struct Qualifier {
        std::string_view name;
        GenericArgList generic_args;
    };

    bool has_generic_args = false;
    IdentInfo* value = nullptr;
    std::span<Qualifier> full_qualification;

    // to keep pre-computed generic instantiation steps to avoid repetitive looping
    std::unordered_map<Type*, Type*> type_instantiation_map;

    explicit Ident()
        : Node(ND_IDENT) {}

    explicit Ident(IdentInfo* val)
        : Ident() { value = val; }

    /// Convenience constructor for the creation of temporary objects, not to be used for
    /// the construction of AST nodes
    explicit Ident(const std::span<Qualifier> full_qualification)
        : full_qualification(full_qualification) {}


    IdentInfo* getIdentInfo() override {
        return value;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_IDENT;
    }

    [[nodiscard]] Ident* getIdent() override {
        return this;
    }

    [[nodiscard]]
    std::string toString() const {
        std::string ret;
        for (const auto& [name, generic_args] : full_qualification) {
            ret += std::string(name) + "::";
        } ret.pop_back();
        ret.pop_back();
        return ret;
    }
};


/// Encodes all the necessary information needed to get a `Type*`.
struct TypeWrapper final : Node {
    enum Modifiers { Reference, Pointer };
    Type* type = nullptr;

    bool    is_mutable    = false;
    bool    is_slice      = false;
    bool    is_pointer    = false;
    bool    is_reference  = false;

    Ident*  type_id  = nullptr;

    std::size_t array_size = 0;        // 0 indicates that the type isn't an array
    TypeWrapper* of_type{};            // set in the case of wrapper types (refs, ptr, arrays, slices)

    explicit TypeWrapper()
        : Node(ND_TYPE) {}

    explicit TypeWrapper(Type* ty)
        : Node(ND_TYPE), type(ty) {}

    [[nodiscard]]
    Type* getSwType() override {
        return type;
    }

    [[nodiscard]]
    NodeType getNodeType() const override {
        return ND_TYPE;
    }


    [[nodiscard]]
    std::string getIDStr() const {
        return {};
    }
};

struct GenericArg final : Node {
    explicit GenericArg(): Node(ND_GEN_ARG) {}

    explicit GenericArg(Expression* expr)
        : GenericArg() { value = expr; }

    explicit GenericArg(TypeWrapper* ty)
        : GenericArg() { value = ty; }


    [[nodiscard]]
    bool isType() const {
        return std::holds_alternative<TypeWrapper*>(value);
    }

    [[nodiscard]]
    bool isEmpty() const {
        return std::holds_alternative<std::monostate>(value);
    }

    [[nodiscard]]
    bool isExpression() const {
        return std::holds_alternative<Expression*>(value);
    }

    Expression* getExpr() const {
        return std::get<Expression*>(value);
    }

    TypeWrapper* getType() const {
        return std::get<TypeWrapper*>(value);
    }


private:
    std::variant<std::monostate, Expression*, TypeWrapper*> value;
};


struct Var final : GlobalNode {
    IdentInfo* var_ident = nullptr;
    TypeWrapper* var_type = nullptr;

    Expression* value = nullptr;

    bool initialized = false;
    bool is_const    = false;
    bool is_volatile = false;
    bool is_comptime = false;
    bool is_param    = false;
    bool is_instance_param = false;   // for the special case of `&self` in methods

    explicit Var(): GlobalNode(ND_VAR) {}

    [[nodiscard]] NodeType getNodeType()  const override { return ND_VAR; }

    IdentInfo* getIdentInfo() override {
        return var_ident;
    }

    [[nodiscard]] Node* getExprValue() override { return value->expr; }
};


struct Scope final : Node {
    Scope*     parent_scope = nullptr;
    Namespace* symbols = nullptr;

    std::span<Node*> children;

    explicit Scope()
        : Node(ND_SCOPE) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_SCOPE;
    }
};


struct Function final : GlobalNode {
    bool is_static_method = true;
    IdentInfo* ident = nullptr;

    std::span<Var*>  params;
    Scope*           children = nullptr;

    TypeWrapper* return_type = nullptr;

    explicit Function()
        : GlobalNode(ND_FUNC) {}

    IdentInfo* getIdentInfo() override {
        return ident;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_FUNC;
    }
};


struct FuncCall final : Node {
    Ident* ident     = nullptr;
    Type*  signature = nullptr;  // supposed to hold the signature of the callee

    std::span<Expression*> args;
    GenericArgList         generic_args;

    explicit FuncCall()
        : Node(ND_CALL) {}

    IdentInfo* getIdentInfo() override {
        return ident->getIdentInfo();
    }

    Type* getSwType() override {
        return signature;
    }

    [[nodiscard]] Ident* getIdent() override {
        return ident;
    }

    [[nodiscard]] NodeType getNodeType() const override { return ND_CALL; }
};


struct Intrinsic final : Node {
    enum Kind { INVALID, SIZEOF, TYPEOF, MEMCPY, MEMSET, ADV_PTR };
    Kind intrinsic_type = INVALID;

    std::span<Expression*> args;
    Ident* ident = nullptr;

    explicit Intrinsic()
        : Node(ND_INTRINSIC) {}

    void operator=(FuncCall* call) {
        args  = call->args;
        ident = call->ident;

        static const std::unordered_map<std::string_view, Kind> tag_map = {
            {"sizeof", SIZEOF},
            {"typeof", TYPEOF},
            {"memset", MEMSET},
            {"memcpy", MEMCPY},
            {"advance_pointer", ADV_PTR}
        }; intrinsic_type = tag_map.at(ident->full_qualification.at(0).name);
    }

    [[nodiscard]] NodeType getNodeType() const override { return ND_INTRINSIC; }
};


struct ImportNode final : GlobalNode {
    struct ImportedSymbol_t {
        std::string_view actual_name;
        std::string_view assigned_alias;
    };

    bool is_wildcard = false;

    sw::FileHandle*  mod_handle = nullptr;
    std::string_view alias;
    std::span<ImportedSymbol_t> imported_symbols{};

    explicit ImportNode()
        : GlobalNode(ND_IMPORT) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_IMPORT;
    }
};


struct ArrayLit final : Node {
    Type* type = nullptr;
    std::span<Expression*> elements;

    explicit ArrayLit()
        : Node(ND_ARRAY) {}

    Type* getSwType() override {
        return type;
    }

    [[nodiscard]]
    bool isLiteral() const override {
        return true;
    }
};


struct WhileLoop final : Node {
    Expression* condition = nullptr;
    Scope* children = nullptr;

    explicit WhileLoop()
        : Node(ND_WHILE) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_WHILE;
    }
};


struct BreakStmt final : Node {
    explicit BreakStmt(): Node(ND_BREAK) {}

    [[nodiscard]] NodeType getNodeType() const override { return ND_BREAK;}
};


struct ContinueStmt final : Node {
    explicit ContinueStmt(): Node(ND_CONTINUE) {}

    [[nodiscard]] NodeType getNodeType() const override { return ND_CONTINUE; }
};


struct Enum final : GlobalNode {
    IdentInfo* ident;

    std::optional<TypeWrapper*> enum_type;
    std::unordered_map<std::string_view, int> entries;

    explicit Enum()
        : GlobalNode(ND_ENUM)
        , ident(nullptr) {}

    int counter = 0;
    void addEntry(const std::string_view id) {
        entries.emplace(id, counter++);
    }
};


struct Protocol final : GlobalNode {
    struct MethodSignature {
        std::string_view name;
        TypeWrapper*     return_type;
        std::span<TypeWrapper*> params;


        bool operator==(const MethodSignature& other) const {
            return name == other.name && other.return_type->type == return_type->type &&
                std::equal(params.begin(), params.end(),
                    other.params.begin(), other.params.end(),
                    [](const TypeWrapper* a, const TypeWrapper* b) {
                        return a->type == b->type;
                    });
        }
    };

    struct MemberSignature {
        std::string_view name;
        TypeWrapper*     type;

        bool operator==(const MemberSignature& other) const {
            return name == other.name && other.type->type == type->type;
        }
    };

    IdentInfo*  protocol_id = nullptr;

    std::span<Ident*> depended_protocols;
    std::span<MemberSignature> members;
    std::span<MethodSignature> methods;

    IdentInfo* getIdentInfo() override {
        return protocol_id;
    }

    explicit Protocol()
        : GlobalNode(ND_PROTOCOL) {}

    [[nodiscard]]
    NodeType getNodeType() const override { return ND_PROTOCOL; }
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
        return combineHashes(std::hash<const char*>{}(m.name.data()), std::hash<TypeWrapper*>{}(m.type));
    }
};

template <>
struct std::hash<Protocol::MethodSignature> {
    std::size_t operator()(const Protocol::MethodSignature& m) const noexcept {
        std::size_t arg_hash = 0;
        for (auto& arg : m.params) {
            arg_hash = combineHashes(arg_hash, std::hash<TypeWrapper*>{}(arg));
        }

        return combineHashes(
            std::hash<const char*>{}(m.name.data()),
            std::hash<TypeWrapper*>{}(m.return_type),
            arg_hash
            );
    }
};


struct Struct final : GlobalNode {
    IdentInfo* ident   = nullptr;
    Scope*     members = nullptr;

    std::span<Ident*>  protocols;

    explicit Struct() : GlobalNode(ND_STRUCT) {}

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_STRUCT;
    }

    IdentInfo* getIdentInfo() override {
        return ident;
    }
};


struct Condition final : Node {
    bool        is_comptime = false;
    Expression* bool_expr = nullptr;

    using elif_t = std::tuple<Expression*, Scope*>;

    Scope*  if_children{};
    std::span<elif_t> elif_children{};
    Scope*  else_children{};

    explicit Condition()
        : Node(ND_COND) {}


    [[nodiscard]] Node* getExprValue() override {
        return bool_expr->expr;
    }

    [[nodiscard]] NodeType getNodeType() const override {
        return ND_COND;
    }
};