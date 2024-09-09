#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <stack>
#include <variant>
#include <unordered_map>
#include <unordered_set>
#include <format>

#include <parser/parser.h>


bool EnablePanicMode = false; // whether the parser is in panik

struct TypeInfo {
    bool is_const = false;
    std::optional<llvm::Value*> gen_val = std::nullopt;
};


std::size_t ScopeIndex{};
std::stack<Node*> ScopeTrack{};
std::vector<std::unique_ptr<Node>> ParsedModule{};

extern const std::unordered_map<std::string, uint8_t> valid_expr_bin_ops;
extern std::unordered_map<std::string, int> operators;

extern void printIR();

void pushToModule(std::unique_ptr<Node> node, const bool isParent = false) {
    if (!ScopeTrack.empty()) node->setParent(ScopeTrack.top());
    ParsedModule.emplace_back(std::move(node));
    if (isParent) {
        ScopeTrack.push(ParsedModule.back().get());
    }
}



Parser::Parser(TokenStream& tks) : m_Stream(tks) {}
Parser::~Parser() = default;


int minEditDistance(const std::string_view word1, const std::string_view word2) {
    std::size_t m = word1.size();
    std::size_t n = word2.size();

    std::vector dp(m + 1, std::vector(n + 1, 0));

    for (int i = 0; i <= m; ++i) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= n; ++j) {
        dp[0][j] = j;
    }

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

void Parser::forwardStream(const uint8_t n = 1) {
    for (uint8_t _ = 0; _ < n; _++)
        m_Stream.next();
}

std::unique_ptr<Node> Parser::dispatch() {
    while (!m_Stream.eof()) {
        const TokenType type = m_Stream.p_CurTk.type;
        const auto value = m_Stream.p_CurTk.value;

        switch (m_Stream.p_CurTk.type) {
            case KEYWORD:
                if (m_Stream.p_CurTk.value == "const" || m_Stream.p_CurTk.value == "var") {
                    auto ret = parseVar();
                    return std::move(ret);
                }
                if (m_Stream.p_CurTk.value == "fn") {
                    ScopeIndex++;
                    auto ret = parseFunction();
                    ret->scope_id = ScopeIndex;
                    return std::move(ret);
                }
                if (m_Stream.p_CurTk.value == "if") {
                    ScopeIndex++;
                    auto ret = parseCondition();
                    ret->scope_id = ScopeIndex;
                    return std::move(ret);
                }
            case IDENT:
                if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(")
                    return parseCall();

                if (m_Stream.peek().type == OP && m_Stream.peek().value == "=") {
                    auto assignment = std::make_unique<Assignment>();
                    assignment->ident = m_Stream.p_CurTk.value;

                    forwardStream(2);
                    parseExpr(&assignment->value);
                    return assignment;
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
    //     std::cout << m_Stream.p_CurTk.value << " peek: " << m_Stream.peek().value << std::endl;
    //     m_Stream.next();
    // }
    forwardStream();

    while (!m_Stream.eof()) {
        ParsedModule.emplace_back(dispatch());
    }

    for ( auto & a : ParsedModule) {
        // a->print();
        a->codegen();
    }
    // m_ExceptionHandler.raiseAll();
    printIR();
}


std::unique_ptr<Function> Parser::parseFunction() {
    Function func_nd{};
    func_nd.ident = m_Stream.next().value;

    // Check for errors
    forwardStream(2);
    static auto parse_params = [this] {
        decltype(func_nd.getParamInstance()) param{};
        param.var_ident = m_Stream.p_CurTk.value; // parameter ident

        forwardStream(2);
        param.var_type = m_Stream.p_CurTk.value;

        param.initialized = m_Stream.peek().type == PUNC && m_Stream.peek().value == "=";
        m_Stream.next();
        return param;
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
    if (m_Stream.p_CurTk.type == IDENT)
        func_nd.ret_type = m_Stream.p_CurTk.value;

    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ":") {
        func_nd.ret_type = m_Stream.next().value;
        forwardStream();
    }

    // parses the function's children and the return statement
    forwardStream();
    while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}")) {
        if (m_Stream.p_CurTk.type == KEYWORD && m_Stream.p_CurTk.value == "return") {
            m_Stream.next();
            if (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ";")) {
                std::cout << "passing control at " << m_Stream.p_CurTk.value << std::endl;
                parseExpr(&func_nd.return_val);
                std::cout << func_nd.return_val.expr.size() << std::endl;
            } continue;
        } func_nd.children.push_back(std::move(dispatch()));
    } forwardStream();

    std::cout << "function leaving at: " << m_Stream.p_CurTk.value << std::endl;
    ScopeIndex--;
    return std::make_unique<Function>(std::move(func_nd));
}


std::unique_ptr<Var> Parser::parseVar() {
    Var var_node;
    var_node.is_const = m_Stream.p_CurTk.value[0] == 'c';
    var_node.var_ident = m_Stream.next().value;


    auto p_token = m_Stream.peek();
    if (p_token.type == PUNC && p_token.value == ":") {
        m_Stream.next();
        var_node.var_type = m_Stream.next().value;
    }

    p_token = m_Stream.peek();
    if (p_token.type == OP && p_token.value == "=") {
        var_node.initialized = true;
        forwardStream(2);
        parseExpr(&var_node.value);
    }

    return std::make_unique<Var>(std::move(var_node));
}


std::unique_ptr<Node> Parser::parseCall() {
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
        parseExpr(&call_node->args, true);
    }

    std::cout << "loop breaking at " << m_Stream.p_CurTk.value << std::endl;
    forwardStream();
    std::cout << "loop breaking at " << m_Stream.p_CurTk.value << std::endl;

    return call_node;
}

std::unique_ptr<Condition> Parser::parseCondition() {
    Condition cnd;
    forwardStream();  // skip "if"
    parseExpr(&cnd.bool_expr);
    forwardStream();  // skip the opening brace

    ScopeIndex--;
    while (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == "}"))
        cnd.if_children.push_back(std::move(dispatch()));
    forwardStream();

    // handle `else(s)`
    if (!(m_Stream.p_CurTk.type == KEYWORD && (m_Stream.p_CurTk.value == "else" || m_Stream.p_CurTk.value == "elif")))
        return std::make_unique<Condition>(std::move(cnd));

    if (m_Stream.p_CurTk.type == KEYWORD && m_Stream.p_CurTk.value == "elif") {
        while (m_Stream.p_CurTk.type == KEYWORD && m_Stream.p_CurTk.value == "elif") {
            forwardStream();

            std::tuple<Expression, std::vector<std::unique_ptr<Node>>> children{};
            parseExpr(&std::get<0>(children));

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


/* This method is an adaptation of the `Shunting Yard Algorithm`. */
void Parser::parseExpr(std::variant<std::vector<Expression>*, Expression*> ptr, bool isCall) {
    // TODO: remove overcomplexity

    bool kill_yourself = false;
    std::stack<Op> ops{};  // our operator stack
    std::vector<std::unique_ptr<Node> > output{}; // the final postfix (RPN) form

    int paren_counter = 0;
    int ops_opr_consumed = 0;

    Token prev_token = {OP, "="};
    static const std::unordered_set invalid_prev_types{IDENT, NUMBER, KEYWORD, STRING};

    while (!m_Stream.eof()) {
        Op top_elem;

        // break once the expression ends
        if (m_Stream.p_CurTk.type == PUNC) {
            if (m_Stream.p_CurTk.value == ",") {
                // for function parameters
                m_Stream.next();
                break;
            }
            if (m_Stream.p_CurTk.value == "}")
                break;
        }

        if (m_Stream.p_CurTk.type == KEYWORD || (prev_token.type != OP && (m_Stream.p_CurTk.type == IDENT)))
            break;
        // if (prev_token.type != OP && m_Stream.p_CurTk.type == IDENT) break;

        if (ops_opr_consumed > 1) {
            if (m_Stream.p_CurTk.type == KEYWORD) break;
            if ((invalid_prev_types.contains(prev_token.type) && m_Stream.p_CurTk.type != OP)) {
                if (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")) { break; }
            }
        }

        switch (m_Stream.p_CurTk.type) {
            case NUMBER:
                output.emplace_back(std::make_unique<IntLit>(IntLit(m_Stream.p_CurTk.value)));
                break;
            case STRING:
                output.emplace_back(std::make_unique<StrLit>(m_Stream.p_CurTk.value));
                break;
            case IDENT:
                if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(") {
                    output.emplace_back(parseCall());
                    continue;
                }
                output.emplace_back(std::make_unique<Ident>(Ident(m_Stream.p_CurTk.value)));
                break;
            case OP:
                // pop ops and push them into output till the top operator of the stack has greater or equal precedence
                while (!ops.empty() && operators[m_Stream.p_CurTk.value] <= operators[ops.top().getValue()]) {
                    output.emplace_back(std::make_unique<Op>(ops.top()));
                    ops.pop();
                }
                ops.emplace(m_Stream.p_CurTk.value);
                break;
            case PUNC:
                if (m_Stream.p_CurTk.value == "(") {
                    paren_counter++;
                    ops.emplace("(");
                    break;
                } if (m_Stream.p_CurTk.value == ")") {
                    paren_counter--;
                    if (isCall && paren_counter == -1) {
                        kill_yourself = true;
                        break;
                    }
                    while (ops.top().getValue() != "(") {
                        output.emplace_back(std::make_unique<Op>(ops.top()));
                        ops.pop();
                    }
                    ops.pop();
                    break;
                }
                break;
            default:
                break;
        }

        if (kill_yourself) {
            kill_yourself = false;
            break;
        }
        ops_opr_consumed++;
        prev_token = m_Stream.p_CurTk;
        m_Stream.next();
    }

    while (!ops.empty()) {
        output.emplace_back(std::make_unique<Op>(ops.top()));
        ops.pop();
    }

    Expression expr{};
    expr.expr.reserve(output.size());
    for (auto& nd: output) {
        auto n = std::make_unique<Node>();
        n = std::move(nd);
        expr.expr.push_back(std::move(n));
    }

    if (std::holds_alternative<std::vector<Expression>*>(ptr))
        std::get<std::vector<Expression> *>(ptr)->push_back(std::move(expr));
    else
        std::get<Expression*>(ptr)->expr = std::move(expr.expr);

    // NOTE: this function propagates the stream at the token right after the expression
    std::cout << "expr leaving at: " << m_Stream.p_CurTk.value << std::endl;
}


