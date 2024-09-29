#pragma once
#include <memory>
#include <utility>
#include <variant>
#include <stack>

#include <backend/LLVMBackend.h>
#include <tokenizer/Tokenizer.h>
#include <llvm/IR/Value.h>
#include <exception/ExceptionHandler.h>
#include <llvm/IR/Instructions.h>
#include <tokens/Tokens.h>


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
    ND_ADDR,        // 15
    ND_DEREF,       // 16
};

struct Var;

// A common base class for all the nodes
struct Node {
    using inst_ptr_t = llvm::AllocaInst*;
    using symt_t     = std::unordered_map<std::string, inst_ptr_t>;

    std::string value{};
    std::size_t scope_id{};

    virtual bool hasScopes() { return false; }
    virtual const std::vector<std::unique_ptr<Node>>& getExprValue() { throw std::runtime_error("getExprValue called on Node instance"); }
    virtual std::string getValue() const { throw std::runtime_error("getValue called on base node"); }
    virtual NodeType getType() const { return ND_INVALID; }
    virtual const std::vector<Var>& getParams() const { throw std::runtime_error("getParams called on base getParams"); }
    virtual llvm::Value* llvmCodegen(LLVMBackend& instance) { return nullptr; }
    virtual int8_t getArity() { throw std::runtime_error("getArity called on base Node instance "); }
    virtual std::size_t getScopeID() const { return scope_id; }
    virtual void print() { throw std::runtime_error("debug called on base Node"); }
    virtual ~Node() = default;
};


struct Op final : Node {
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

struct Expression final : Node {
    std::vector<std::unique_ptr<Node>> expr{};
    std::string expr_type{};

    Expression() = default;
    Expression(Expression&& other) noexcept {
        expr.reserve(other.expr.size());
        this->expr_type = std::move(other.expr_type);

        std::move(
                std::make_move_iterator(other.expr.begin()),
                std::make_move_iterator(other.expr.end()),
                std::back_inserter(expr));

        for (const auto& nd : expr) {
//            std::cout << "moving : " << nd->getValue() << std::endl;
        }
    }

    Expression& operator=(Expression&& other) noexcept {
        expr.clear();
        expr.reserve(other.expr.size());
        std::move(
                std::make_move_iterator(other.expr.begin()),
                std::make_move_iterator(other.expr.end()),
                std::back_inserter(expr));
        return *this;
    }

    const std::vector<std::unique_ptr<Node>>& getExprValue() override { return expr; }

    std::string getValue() const override {
        throw std::runtime_error("getValue called on expression");
    }

    NodeType getType() const override {
        return ND_EXPR;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct Assignment final : Node {
    Expression  l_value{};
    std::string op{};
    Expression r_value{};

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;

    NodeType getType() const override {
        return ND_ASSIGN;
    }
};

struct ReturnStatement final : Node {
    Expression value{};
    std::string parent_ret_type{};

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;

    NodeType getType() const override {
        return ND_RET;
    }

};

struct IntLit final : Node {
    std::string value;

    explicit IntLit(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_INT;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};

struct FloatLit final : Node {
    std::string value = 0;

    explicit FloatLit(std::string val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_FLOAT;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct StrLit final : Node {
    std::string value;

    explicit StrLit(std::string  val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_STR;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};

struct Ident final : Node {
    std::string value;

    explicit Ident(std::string  val): value(std::move(val)) {}

    std::string getValue() const override {
        return value;
    }

    NodeType getType() const override {
        return ND_IDENT;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct Var final : Node {
    std::string var_ident;
    std::string var_type;
    Expression value;
    Node* parent = nullptr;

    bool initialized = false;
    bool is_const    = false;
    bool is_volatile = false;

    Var() = default;

    std::string getValue() const override { return var_ident; }
    NodeType getType() const override { return ND_VAR; }

    void print() override;

    std::vector<std::unique_ptr<Node>>& getExprValue() override { return value.expr; }
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct Function final : Node {
    std::string ident;
    std::string ret_type = "void";

    std::vector<Var> params{};
    std::vector<std::unique_ptr<Node>> children{};

    NodeType getType() const override {
        return ND_FUNC;
    }

    std::string getValue() const override {
        return ident;
    }

    const std::vector<Var>& getParams() const override {
        return params;
    }

    bool hasScopes() override {
        return true;
    }

    void print() override;

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct FuncCall final : Node {
    std::vector<Expression> args;
    std::string ident;
    std::string type = "void";
    std::string getValue() const override { return ident; }
    Node* parent = nullptr;

    NodeType     getType() const override { return ND_CALL; }
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

struct WhileLoop final : Node {
    Expression condition{};
    std::vector<std::unique_ptr<Node>> children{};

    NodeType getType() const override {
        return ND_WHILE;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};


struct Struct final : Node {
    std::string ident{};
    std::vector<std::unique_ptr<Node>> members{};

    NodeType getType() const override {
        return ND_STRUCT;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};

struct AddressOf final : Node {
    std::string ident{};

    NodeType getType() const override {
        return ND_ADDR;
    }

    std::string getValue() const override {
        return "&" + ident;
    }

    llvm::Value *llvmCodegen(LLVMBackend& instance) override;
};


struct Dereference final : Node {
    std::string ident{};

    NodeType getType() const override {
        return ND_DEREF;
    }

    std::string getValue() const override {
        return "@" + ident;
    }

    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
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

    NodeType getType() const override {
        return ND_COND;
    }

    void print() override;
    llvm::Value* llvmCodegen(LLVMBackend& instance) override;
};

class Parser {
    Token cur_rd_tok{};
    ExceptionHandler m_ExceptionHandler{};
    std::string m_LatestFunctRetType{};
    LLVMBackend m_LLVMInstance;

public:
    TokenStream m_Stream;

    bool m_AppendToScope = false;
    bool m_TokenToScope  = false;
    std::vector<std::string> registered_symbols{};

    explicit Parser(TokenStream);

    std::unique_ptr<Function> parseFunction();
    std::unique_ptr<Condition> parseCondition();
    std::unique_ptr<Node> parseCall();
    std::unique_ptr<Node> dispatch();
    std::unique_ptr<Var> parseVar(bool is_volatile = false);
    std::unique_ptr<WhileLoop> parseWhile();
    std::unique_ptr<ReturnStatement> parseRet();
    std::unique_ptr<Struct> parseStruct();

    void forwardStream(uint8_t n = 1);
    void parseExpr(std::variant<std::vector<Expression>*, Expression*>, bool isCall = false, std::optional<std::string> as_type = std::nullopt);
    void parse();
//    void appendAST(Node&);
    inline void next(bool swsFlg = false, bool snsFlg = false);

    ~Parser();

    std::string parseType() {
        std::string ret = m_Stream.p_CurTk.value;
        forwardStream();

        if (m_Stream.p_CurTk.type == OP && m_Stream.p_CurTk.value.starts_with("*")) {
            ret += m_Stream.p_CurTk.value;
            forwardStream();
        }

        std::cout << "ParseType leaving at -> " << m_Stream.p_CurTk.value << ": " << m_Stream.p_CurTk.type << std::endl;
        return ret;
    }
};
