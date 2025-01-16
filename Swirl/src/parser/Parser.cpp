#include <iostream>
#include <memory>
#include <utility>
#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>

#include <utils/utils.h>
#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <definitions.h>
#include <filesystem>


using SwNode = std::unique_ptr<Node>;


extern std::string SW_OUTPUT;

extern void printIR(const LLVMBackend& instance);
extern void GenerateObjectFileLLVM(const LLVMBackend&);

Parser::~Parser() = default;

Parser::Parser(std::string src): m_Stream{std::move(src)}, m_ErrMan{m_Stream} {
    m_Stream.ErrMan = &m_ErrMan;
}

Token Parser::forwardStream(const uint8_t n) {
    Token ret = m_Stream.CurTok;
    for (uint8_t _ = 0; _ < n; _++)
        m_Stream.next();
    return std::move(ret);
}


Type* Parser::parseType() {
    ParsedType pt;

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "&") {
        pt.is_reference = true;
        forwardStream();
    }

    pt.ident = SymbolTable.getIDInfoFor(forwardStream().value);
    return SymbolTable.lookupType(pt.ident);

    // TODO handle pointers and references
}

SwNode Parser::dispatch() {
    while (!m_Stream.eof()) {
        const TokenType type  = m_Stream.CurTok.type;
        const std::string value = m_Stream.CurTok.value;

        switch (m_Stream.CurTok.type) {
            case KEYWORD:
                // pattern matching in C++ when?
                if (m_Stream.CurTok.value == "const" || m_Stream.CurTok.value == "var") {
                    auto ret = parseVar(false);
                    return std::move(ret);
                }
                if (m_Stream.CurTok.value == "fn") {
                    auto ret = parseFunction();
                    return std::move(ret);
                }
                if (m_Stream.CurTok.value == "if") {
                    auto ret = parseCondition();
                    return std::move(ret);
                }

                if (m_Stream.CurTok.value == "struct")
                    return parseStruct();

                if (m_Stream.CurTok.value == "while")
                    return parseWhile();

                if (m_Stream.CurTok.value == "volatile") {
                    forwardStream();
                    return parseVar(true);
                }

                if (m_Stream.CurTok.value == "return") {
                    return parseRet();
                }

            case IDENT:
                if (const Token p_tk = m_Stream.peek();
                    p_tk.type == PUNC && p_tk.value == "(") {
                    auto nd = parseCall();

                    // handle call assignments
                    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value.ends_with("=")) {
                        Assignment ass{};
                        ass.op = m_Stream.CurTok.value;
                        ass.l_value.expr.push_back(std::move(nd));
                        forwardStream();
                        ass.r_value = parseExpr();

                        return std::make_unique<Assignment>(std::move(ass));
                    }

                    return std::move(nd);
                }

                {
                    Expression expr = parseExpr();

                    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "=") {
                        std::cout << "ass detected" << std::endl;
                        Assignment ass;
                        ass.l_value = std::move(expr);

                        forwardStream();
                        ass.r_value = parseExpr();
                        return std::make_unique<Assignment>(std::move(ass));
                    } continue;
                }
            case OP:
                assignment_lhs:
                {
                    Expression expr = parseExpr();
                    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "=") {
                        std::cout << "ass detected (OP)" << std::endl;
                        Assignment ass{};
                        ass.l_value = std::move(expr);

                        forwardStream();  // skip "="
                        ass.r_value = parseExpr();
                        return std::make_unique<Assignment>(std::move(ass));
                    }  continue;
                }
            case PUNC:
                if (m_Stream.CurTok.value == "(")
                    goto assignment_lhs;  // sorry
                if (m_Stream.CurTok.value == ";") {
                    forwardStream();
                    continue;
                } if (m_Stream.CurTok.value == "}") {
                    std::cout << "Warning: emtpy node returned" << std::endl;
                    return std::make_unique<Node>();
                }
            default:
                auto [line, _, col] = m_Stream.getStreamState();
                std::cout << m_Stream.CurTok.value << ": " << type << std::endl;
                std::cout << "Line: " << line << " Col: " << col << std::endl;
                throw std::runtime_error("dispatch: nothing found");
        }
    }

    throw std::runtime_error("dispatch: this wasn't supposed to happen");
}


void Parser::parse() {
    // uncomment to check the stream's output for debugging
    // while (!m_Stream.eof()) {
    //     std::cout << m_Stream.p_CurTk.value << ": " << to_string(m_Stream.p_CurTk.type) << " peek: " << m_Stream.peek().value << std::endl;
    //     m_Stream.next();
    // }

    forwardStream();
    while (!m_Stream.eof()) {
        ParsedModule.emplace_back(dispatch());
    }
}



void Parser::callBackend() {
    SymbolTable.lockNewScpEmplace();
    LLVMBackend llvm_instance{"deme", std::move(SymbolTable), std::move(m_ErrMan)};  // TODO
    for (const auto& a : ParsedModule) {
        // a->print();
        a->llvmCodegen(llvm_instance);
    }

    printIR(llvm_instance);
    GenerateObjectFileLLVM(llvm_instance);
}

/// returns whether `t1` is implicitly convertible to `t2`
bool implicitlyConvertible(Type* t1, Type* t2) {
    // TODO: handled signed-ness
    if (t1->isIntegral() && t2->isIntegral()) {
        return t1->getBitWidth() <= t2->getBitWidth();
    }
    return false;
}

std::unique_ptr<Function> Parser::parseFunction() {
    Function func_nd;
    m_Stream.expectTypes({IDENT});
    const std::string func_ident = m_Stream.next().value;

    m_Stream.expectTokens({Token{PUNC, "("}});
    forwardStream(2);
    auto function_t = std::make_unique<FunctionType>();
    function_t->ident = func_nd.ident;
    func_nd.reg_ret_type = &function_t->ret_type;

    SymbolTable.newScope();

    static auto parse_params = [&, this] {
        Var param;
        const std::string var_name = m_Stream.CurTok.value;

        forwardStream(2);
        param.var_type = parseType();

        param.initialized = m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "=";
        if (param.initialized)
            param.value = parseExpr();
        function_t->param_types.push_back(param.var_type);

        TableEntry param_entry;
        param_entry.swirl_type = param.var_type;
        param_entry.is_param = true;

        param.var_ident = SymbolTable.registerDecl(var_name, param_entry);

        return std::move(param);
    };

    if (m_Stream.CurTok.type != PUNC) {
        while (m_Stream.CurTok.value != ")" && m_Stream.CurTok.type != PUNC) {
            func_nd.params.push_back(parse_params());
            if (m_Stream.CurTok.value == ",")
                forwardStream();
        }
    }

    // current token == ')'
    forwardStream();

    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ":") {
        forwardStream();
        func_nd.ret_type = parseType();
        function_t->ret_type = func_nd.ret_type;
    }

    TableEntry entry;
    entry.swirl_type = function_t.get();
    func_nd.ident = SymbolTable.registerDecl(func_ident, entry);
    SymbolTable.registerType(func_nd.ident, function_t.release());

    // WARNING: func_nd has been MOVED here!!
    auto ret = std::make_unique<Function>(std::move(func_nd));
    // ReSharper disable once CppDFALocalValueEscapesFunction
    m_LatestFuncNode = ret.get();

    // parse the children
    forwardStream();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        ret->children.push_back(dispatch());
    } forwardStream();
    SymbolTable.destroyLastScope();

    return std::move(ret);
}

std::unique_ptr<Var> Parser::parseVar(const bool is_volatile) {
    Var var_node;
    var_node.is_const = m_Stream.CurTok.value[0] == 'c';
    var_node.is_volatile = is_volatile;
    const std::string var_ident = m_Stream.next().value;
    forwardStream();  // [:, =]


    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ":") {
        forwardStream();
        var_node.var_type = parseType();
    }

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "=") {
        var_node.initialized = true;
        forwardStream();
        var_node.value = parseExpr(var_node.var_type);
    }

    TableEntry entry;
    entry.is_const = var_node.is_const;
    entry.is_volatile = var_node.is_volatile;

    if (!var_node.var_type)
        var_node.var_type = var_node.value.expr_type;
    else
        var_node.value.expr_type = var_node.var_type;

    entry.swirl_type = var_node.var_type;
    var_node.var_ident = SymbolTable.registerDecl(var_ident, entry);

    return std::make_unique<Var>(std::move(var_node));
}

SwNode Parser::parseCall() {
    auto call_node = std::make_unique<FuncCall>();
    call_node->ident = SymbolTable.getIDInfoFor(m_Stream.CurTok.value);
    forwardStream(2);

    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ")") {
        forwardStream();
        return call_node;
    }

    while (true) {
        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ")")
            break;

        call_node->args.emplace_back();
        call_node->args.back() = parseExpr({});
    }

    forwardStream();

    return call_node;
}

std::unique_ptr<ReturnStatement> Parser::parseRet() {
    ReturnStatement ret;

    forwardStream();
    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ";")
        return std::make_unique<ReturnStatement>(std::move(ret));

    ret.value = parseExpr();
    if (m_LatestFuncNode->ret_type == nullptr) {
        m_LatestFuncNode->updateRetTypeTo(ret.value.expr_type);
        ret.parent_ret_type = ret.value.expr_type;
    }
    else {
        if (!implicitlyConvertible(ret.value.expr_type, m_LatestFuncNode->ret_type)) {
            throw std::runtime_error("returned value type-mismatch");
        }
        ret.value.expr_type = m_LatestFuncNode->ret_type;
        ret.parent_ret_type = m_LatestFuncNode->ret_type;
    }

    return std::make_unique<ReturnStatement>(std::move(ret));
}


std::unique_ptr<Condition> Parser::parseCondition() {
    Condition cnd;
    forwardStream();  // skip "if"
    cnd.bool_expr = parseExpr();
    forwardStream();  // skip the opening brace

    SymbolTable.newScope();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}"))
        cnd.if_children.push_back(std::move(dispatch()));
    forwardStream();
    SymbolTable.destroyLastScope();

    // handle `else(s)`
    if (!(m_Stream.CurTok.type == KEYWORD && (m_Stream.CurTok.value == "else" || m_Stream.CurTok.value == "elif")))
        return std::make_unique<Condition>(std::move(cnd));

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
        while (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
            SymbolTable.newScope();
            forwardStream();

            std::tuple<Expression, std::vector<SwNode>> children{};
            std::get<0>(children) = parseExpr({});

            forwardStream();
            while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
                std::get<1>(children).push_back(dispatch());
            } forwardStream();

            cnd.elif_children.emplace_back(std::move(children));
            SymbolTable.destroyLastScope();
        }
    }

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "else") {
        SymbolTable.newScope();
        forwardStream(2);
        while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
            cnd.else_children.push_back(dispatch());
        } forwardStream();
        SymbolTable.destroyLastScope();
    }

    return std::make_unique<Condition>(std::move(cnd));
}


std::unique_ptr<Struct> Parser::parseStruct() {
    forwardStream();
    Struct ret{};

    ret.ident = SymbolTable.getIDInfoFor<true>(m_Stream.CurTok.value);

    auto struct_t = std::make_unique<StructType>();
    struct_t->ident = ret.ident;

    forwardStream(2);

    SymbolTable.newScope();
    int mem_index = 0;

    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        auto member = dispatch();

        if (member->getNodeType() == ND_VAR) {
            const auto field_ptr = dynamic_cast<Var*>(member.get());
            struct_t->fields.insert({field_ptr->var_ident, {mem_index, field_ptr->var_type}});
            mem_index++;
        }

        ret.members.push_back(std::move(member));
    }

    forwardStream();
    SymbolTable.destroyLastScope();
    SymbolTable.registerType(ret.ident, struct_t.release());
    return std::make_unique<Struct>(std::move(ret));
}

std::unique_ptr<WhileLoop> Parser::parseWhile() {
    WhileLoop loop{};
    forwardStream();

    loop.condition = parseExpr();

    SymbolTable.newScope();
    forwardStream();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        loop.children.push_back(dispatch());
    } forwardStream();
    SymbolTable.destroyLastScope();

    return std::make_unique<WhileLoop>(std::move(loop));
}


void deduceType(Type** placed_into, Type* from) {
    // TODO: signed types
    if (*placed_into == nullptr)
        *placed_into = from;
    else {
        if ((*placed_into)->isFloatingPoint() &&
            from->getBitWidth() > (*placed_into)->getBitWidth()) {
            *placed_into = from;
            }
        if ((*placed_into)->isIntegral() &&
            from->getBitWidth() > (*placed_into)->getBitWidth()) {
            *placed_into = from;
            }
    }
}


Expression Parser::parseExpr(const std::optional<Type*>) {
    Type* deduced_type = nullptr;

    const
    std::function<SwNode()> parse_component = [&, this] -> SwNode {
        switch (m_Stream.CurTok.type) {
            case NUMBER: {
                if (m_Stream.CurTok.meta == CT_FLOAT) {
                    auto ret = std::make_unique<FloatLit>(m_Stream.CurTok.value);
                    forwardStream();
                    deduced_type = SymbolTable.lookupType("f64");
                    return std::move(ret);
                }

                auto ret = std::make_unique<IntLit>(m_Stream.CurTok.value);

                if (!deduced_type) {
                    deduced_type = SymbolTable.lookupType("i32");
                }


                forwardStream();
                return std::move(ret);
            }

            case IDENT: {
                if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(") {
                    auto call_node = parseCall();
                    Type* fn_ret_type = dynamic_cast<FunctionType*>(SymbolTable.lookupDecl(call_node->getIdentInfo()).swirl_type)->ret_type;
                    deduceType(&deduced_type, fn_ret_type);
                    return std::move(call_node);
                }

                auto id_node = std::make_unique<Ident>(SymbolTable.getIDInfoFor(forwardStream().value));
                Type* id_type = SymbolTable.lookupDecl(id_node->value).swirl_type;
                deduceType(&deduced_type, id_type);

                return std::move(id_node);
            }
            default: {
                if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "(") {
                    forwardStream();
                    auto ret = std::make_unique<Expression>(parseExpr());
                    deduceType(&deduced_type, ret->expr_type);
                    forwardStream();
                    return std::move(ret);
                } return dispatch();
            }
        }
    };

    const
    std::function<std::optional<SwNode>()> parse_prefix = [&, this] -> std::optional<SwNode> {
        // assumption: current token is an OP
        if (m_Stream.CurTok.type == OP) {
            Op op(m_Stream.CurTok.value);
            op.arity = 1;
            forwardStream();
            if (auto next_op = parse_prefix()) {
                op.operands.push_back(std::move(*next_op));
                return std::make_unique<Op>(std::move(op));
            } return std::nullopt;
        }
        return parse_component();
    };

    const auto continue_parsing = [this]() -> bool {
        return m_Stream.CurTok.type == OP && m_Stream.CurTok.value != "=";
    };

    const
    std::function<SwNode(SwNode, int, bool)> handle_binary =
        [&, this](SwNode prev, const int last_prec = -1, bool is_left_associative = false) {
        // assumption: current token is an OP

        is_left_associative = m_Stream.CurTok.value == "." || m_Stream.CurTok.value == "-";
        SwNode op = std::make_unique<Op>(m_Stream.CurTok.value);
        const int current_prec = operators.at(m_Stream.CurTok.value);

        op->setArity(2);
        forwardStream();  // we are onto the rhs

        if (last_prec < 0) {
            auto rhs = m_Stream.CurTok.type == OP ? parse_prefix().value() : parse_component();
            op->getMutOperands().push_back(std::move(prev));
            op->getMutOperands().push_back(std::move(rhs));

            // if there's another operator...
            if (continue_parsing()) {
                if (!op) throw std::invalid_argument("Invalid operation");
                op = handle_binary(std::move(op), current_prec, is_left_associative);
            }
        }

        else if (current_prec < last_prec || is_left_associative) {
            auto rhs = m_Stream.CurTok.type == OP ? parse_prefix().value() : parse_component();

            if (!rhs || !prev) throw std::invalid_argument("rhs || prev: Invalid operation");
            op->getMutOperands().push_back(std::move(prev));
            op->getMutOperands().push_back(std::move(rhs));


            if (continue_parsing())
                op = handle_binary(std::move(op), current_prec, is_left_associative);

            return std::move(op);
        }

        else if (current_prec >= last_prec) {
            SwNode tmp_operand = std::move(prev->getMutOperands().back());
            prev->getMutOperands().pop_back();
            // prepare the higher-precedence Op and push it as an operand
            auto rhs = m_Stream.CurTok.type == OP ? parse_prefix().value() : parse_component();

            if (!tmp_operand || !rhs) throw std::invalid_argument("rhs || prev: Invalid operation");

            op->getMutOperands().push_back(std::move(tmp_operand));
            op->getMutOperands().push_back(std::move(rhs));

            prev->getMutOperands().push_back(std::move(op));

            if (continue_parsing()) {
                op = handle_binary(std::move(prev), current_prec, is_left_associative);
                return std::move(op);
            }
            return std::move(prev);
        }

        return std::move(op);
    };

    Expression ret;
    if (m_Stream.CurTok.type == OP) {
        auto op = parse_prefix().value();

        if (continue_parsing()) {
            ret.expr.push_back(handle_binary(std::move(op), -1, false));
            ret.expr_type = deduced_type;
            return std::move(ret);
        }

        ret.expr.push_back(std::move(op));
        ret.expr_type = deduced_type;
        return std::move(ret);

        
    } else {
        auto tmp = parse_component();
        if (continue_parsing()) {
            ret.expr.push_back(handle_binary(std::move(tmp), -1, false));
            ret.expr_type = deduced_type;
            return std::move(ret);
        }
        ret.expr.push_back(std::move(tmp));
        ret.expr_type = deduced_type;
        return std::move(ret);
    }
}