#include <iostream>
#include <memory>
#include <utility>
#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>

#include <parser/Parser.h>


using SwNode = std::unique_ptr<Node>;

std::vector<SwNode> ParsedModule{};


extern const std::unordered_map<std::string, uint8_t> valid_expr_bin_ops;
extern std::string SW_OUTPUT;

extern void printIR(const LLVMBackend& instance);
extern void GenerateObjectFileLLVM(const LLVMBackend&);


Parser::Parser(TokenStream tks): m_LLVMInstance{SW_OUTPUT}, m_Stream{std::move(tks)} {}
Parser::~Parser() = default;


int minEditDistance(const std::string_view word1, const std::string_view word2) {
    const std::size_t m = word1.size();
    const std::size_t n = word2.size();

    std::vector dp(m + 1, std::vector(n + 1, 0));

    for (int i = 0; i <= m; ++i)
        dp[i][0] = i;

    for (int j = 0; j <= n; ++j)
        dp[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (word1[i - 1] == word2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }

    return dp[m][n];
}

Token Parser::forwardStream(const uint8_t n) {
    Token ret = m_Stream.p_CurTk;
    for (uint8_t _ = 0; _ < n; _++)
        m_Stream.next();
    return std::move(ret);
}

SwNode Parser::dispatch() {
    while (!m_Stream.eof()) {
        const TokenType type  = m_Stream.p_CurTk.type;
        const std::string value = m_Stream.p_CurTk.value;

        switch (m_Stream.p_CurTk.type) {
            case KEYWORD:
                if (m_Stream.p_CurTk.value == "const" || m_Stream.p_CurTk.value == "var") {
                    auto ret = parseVar(false);
                    return std::move(ret);
                }
                if (m_Stream.p_CurTk.value == "fn") {
                    auto ret = parseFunction();
                    return std::move(ret);
                }
                if (m_Stream.p_CurTk.value == "if") {
                    auto ret = parseCondition();
                    return std::move(ret);
                }

                if (m_Stream.p_CurTk.value == "struct")
                    return parseStruct();

                if (m_Stream.p_CurTk.value == "while")
                    return parseWhile();

                if (m_Stream.p_CurTk.value == "volatile") {
                    forwardStream();
                    return parseVar(true);
                }

                if (m_Stream.p_CurTk.value == "return")
                    return parseRet();

            case IDENT:
                if (const Token p_tk = m_Stream.peek();
                    p_tk.type == PUNC && p_tk.value == "(") {
                    auto nd = parseCall();

                    // handle call assignments
                    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value.ends_with("=")) {
                        Assignment ass{};
                        ass.op = m_Stream.p_CurTk.value;
                        ass.l_value.expr.push_back(std::move(nd));
                        forwardStream();
                        ass.r_value = parseExpr("");

                        return std::make_unique<Assignment>(std::move(ass));
                    }

                    return std::move(nd);
                }

                {
                    Expression expr = parseExpr("");

                    if (m_Stream.p_CurTk.type == OP && m_Stream.p_CurTk.value == "=") {
                        std::cout << "ass detected" << std::endl;
                        Assignment ass{};
                        ass.l_value = std::move(expr);

                        forwardStream();
                        ass.r_value = parseExpr("");
                        return std::make_unique<Assignment>(std::move(ass));
                    } continue;
                }
            case OP:
                assignment_lhs:
                {
                    Expression expr = parseExpr("");
                    if (m_Stream.p_CurTk.type == OP && m_Stream.p_CurTk.value == "=") {
                        std::cout << "ass detected (OP)" << std::endl;
                        Assignment ass{};
                        ass.l_value = std::move(expr);

                        forwardStream();  // skip "="
                        ass.r_value = parseExpr("");
                        return std::make_unique<Assignment>(std::move(ass));
                    }  continue;
                }
            case PUNC:
                if (m_Stream.p_CurTk.value == "(")
                    goto assignment_lhs;

                if (m_Stream.p_CurTk.value == ";") {
                    forwardStream();
                    continue;
                } if (m_Stream.p_CurTk.value == "}") {
                    std::cout << "Warning: emtpy node returned" << std::endl;
                    return std::make_unique<Node>();
                }
            default:
                auto [line, _, col] = m_Stream.getStreamState();
                std::cout << m_Stream.p_CurTk.value << ": " << type << std::endl;
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

    for (const auto & a : ParsedModule) {
        // a->print();
        a->llvmCodegen(m_LLVMInstance);
    }

    printIR(m_LLVMInstance);
    GenerateObjectFileLLVM(m_LLVMInstance);
}


std::unique_ptr<Function> Parser::parseFunction() {
    Function func_nd{};
    func_nd.ident = m_Stream.next().value;

    std::cout << "parsing function: " << func_nd.ident << std::endl;

    // Check for errors
    forwardStream(2);
    static auto parse_params = [this] {
        Var param{};
        param.var_ident = m_Stream.p_CurTk.value; // parameter ident

        forwardStream(2);
        param.var_type = parseType();

        param.initialized = m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "=";
        if (param.initialized)
            param.value = parseExpr("");

        return std::move(param);
    };

    if (m_Stream.p_CurTk.type != PUNC) {
        while (m_Stream.p_CurTk.value != ")" && m_Stream.p_CurTk.type != PUNC) {
            func_nd.params.push_back(parse_params());
            if (m_Stream.p_CurTk.value == ",")
                m_Stream.next();
        }
    }

    // current token == ')'
    m_Stream.next();

    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ":") {
        forwardStream();
        m_LatestFuncRetType = func_nd.ret_type = parseType();
    }

    // parse the children
    forwardStream();
    while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}")) {
        func_nd.children.push_back(std::move(dispatch()));
    } forwardStream();


    std::cout << "function leaving at: " << m_Stream.p_CurTk.value << std::endl;
    return std::make_unique<Function>(std::move(func_nd));
}


std::unique_ptr<Var> Parser::parseVar(const bool is_volatile) {
    Var var_node;
    var_node.is_const = m_Stream.p_CurTk.value[0] == 'c';
    var_node.is_volatile = is_volatile;
    var_node.var_ident = m_Stream.next().value;
    forwardStream();  // [:, =]

    std::cout << "var: " << var_node.var_ident << std::endl;

    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ":") {
        forwardStream();
        var_node.var_type = parseType();
    }

    if (m_Stream.p_CurTk.type == OP && m_Stream.p_CurTk.value == "=") {
        var_node.initialized = true;
        forwardStream();

        var_node.value = parseExpr(var_node.var_type);
        var_node.value.expr_type = var_node.var_type;
        std::cout << "PrattParseExpr Leaving at: " << m_Stream.p_CurTk.value << std::endl;
        var_node.value.expr_type = var_node.var_type;
        std::cout << "type found: " << var_node.value.expr_type << std::endl;
    }

    return std::make_unique<Var>(std::move(var_node));
}


SwNode Parser::parseCall() {
    auto call_node = std::make_unique<FuncCall>();
    call_node->ident = m_Stream.p_CurTk.value;
    forwardStream(2);

    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")") {
        forwardStream();
        return call_node;
    }

    while (true) {
        if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")
            break;

        call_node->args.emplace_back();
        call_node->args.back() = parseExpr({});
    }

    std::cout << "loop breaking at " << m_Stream.p_CurTk.value << std::endl;
    forwardStream();
    std::cout << "loop breaking at " << m_Stream.p_CurTk.value << std::endl;

    return call_node;
}

std::unique_ptr<ReturnStatement> Parser::parseRet() {
    ReturnStatement ret;

    forwardStream();
    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ";")
        return std::make_unique<ReturnStatement>(std::move(ret));

    ret.parent_ret_type = m_LatestFuncRetType;
    std::cout << "Latest func ret type: " << m_LatestFuncRetType << std::endl;

    ret.value.expr_type = m_LatestFuncRetType;
    ret.value = parseExpr(m_LatestFuncRetType);
    ret.value.expr_type = ret.parent_ret_type;
    return std::make_unique<ReturnStatement>(std::move(ret));
}


std::unique_ptr<Condition> Parser::parseCondition() {
    Condition cnd;
    forwardStream();  // skip "if"
    cnd.bool_expr = parseExpr("bool");
    forwardStream();  // skip the opening brace
    
    while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}"))
        cnd.if_children.push_back(std::move(dispatch()));
    forwardStream();

    // handle `else(s)`
    if (!(m_Stream.p_CurTk.type == KEYWORD && (m_Stream.p_CurTk.value == "else" || m_Stream.p_CurTk.value == "elif")))
        return std::make_unique<Condition>(std::move(cnd));

    if (m_Stream.p_CurTk.type == KEYWORD && m_Stream.p_CurTk.value == "elif") {
        while (m_Stream.p_CurTk.type == KEYWORD && m_Stream.p_CurTk.value == "elif") {
            forwardStream();

            std::tuple<Expression, std::vector<SwNode>> children{};
            std::get<0>(children) = parseExpr({});

            forwardStream();
            while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}")) {
                std::get<1>(children).push_back(dispatch());
            } forwardStream();

            cnd.elif_children.emplace_back(std::move(children));
        }
    }

    if (m_Stream.p_CurTk.type == KEYWORD && m_Stream.p_CurTk.value == "else") {
        forwardStream(2);
        while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}")) {
            cnd.else_children.push_back(dispatch());
        } forwardStream();
    }

    return std::make_unique<Condition>(std::move(cnd));
}


std::unique_ptr<Struct> Parser::parseStruct() {
    forwardStream();
    Struct ret{};

    ret.ident = m_Stream.p_CurTk.value;
    forwardStream(2);

    while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}"))
        ret.members.push_back(dispatch());
    forwardStream();

    return std::make_unique<Struct>(std::move(ret));
}

std::unique_ptr<WhileLoop> Parser::parseWhile() {
    WhileLoop loop{};
    forwardStream();

    loop.condition = parseExpr("bool");
    forwardStream();
    while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}")) {
        loop.children.push_back(dispatch());
    } forwardStream();

    return std::make_unique<WhileLoop>(std::move(loop));
}


Expression Parser::parseExpr(std::string_view type) {
    const
    std::function<SwNode()> parse_component = [this, &type] -> SwNode {
        switch (m_Stream.p_CurTk.type) {
            case NUMBER: {
                if (m_Stream.p_CurTk.meta_ti == CT_FLOAT) {
                    auto ret = std::make_unique<FloatLit>(m_Stream.p_CurTk.value);
                    forwardStream();
                    return std::move(ret);
                }
                auto ret = std::make_unique<IntLit>(m_Stream.p_CurTk.value);
                forwardStream();
                return std::move(ret);
            }
            case IDENT:
                if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(")
                    return parseCall();
                return std::make_unique<Ident>(forwardStream().value);
            default: {
                if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "(") {
                    forwardStream();
                    auto ret = std::make_unique<Expression>(parseExpr({}));
                    ret->expr_type = type;
                    forwardStream();
                    return std::move(ret);
                } return dispatch();
            }
        }
    };

    const
    std::function<std::optional<SwNode>()> parse_prefix = [&, this] -> std::optional<SwNode> {
        // assumption: current token is an OP
        if (m_Stream.p_CurTk.type == OP) {
            Op op(m_Stream.p_CurTk.value);
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
        return m_Stream.p_CurTk.type == OP && m_Stream.p_CurTk.value != "=";
    };

    const
    std::function<SwNode(SwNode, int, bool)> handle_binary =
        [&, this](SwNode prev, const int last_prec = -1, bool is_left_associative = false) {
        // assumption: current token is an OP

        is_left_associative = m_Stream.p_CurTk.value == "." || m_Stream.p_CurTk.value == "-";
        SwNode op = std::make_unique<Op>(m_Stream.p_CurTk.value);
        const int current_prec = operators.at(m_Stream.p_CurTk.value);

        op->setArity(2);
        forwardStream();  // we are onto the rhs

        if (last_prec < 0) {
            auto rhs = m_Stream.p_CurTk.type == OP ? parse_prefix().value() : parse_component();
            op->getMutOperands().push_back(std::move(prev));
            op->getMutOperands().push_back(std::move(rhs));

            // if there's another operator...
            if (continue_parsing()) {
                if (!op) throw std::invalid_argument("Invalid operation");
                op = handle_binary(std::move(op), current_prec, is_left_associative);
            }
        }

        else if (current_prec < last_prec || is_left_associative) {
            auto rhs = m_Stream.p_CurTk.type == OP ? parse_prefix().value() : parse_component();

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
            auto rhs = m_Stream.p_CurTk.type == OP ? parse_prefix().value() : parse_component();

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
    if (m_Stream.p_CurTk.type == OP) {
        auto op = parse_prefix().value();

        if (continue_parsing()) {
            ret.expr.push_back(handle_binary(std::move(op), -1, false));
            return std::move(ret);
        }

        ret.expr.push_back(std::move(op));
        return std::move(ret);
    } else {
        auto tmp = parse_component();
        if (continue_parsing()) {
            ret.expr.push_back(handle_binary(std::move(tmp), -1, false));
            return std::move(ret);
        }
        ret.expr.push_back(std::move(tmp));
        return std::move(ret);
    }
}