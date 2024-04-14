#include <iostream>
#include <memory>
#include <string>
#include <stack>
#include <variant>
#include <unordered_map>
#include <unordered_set>

#include <parser/parser.h>
#include <llvm/IR/Type.h>
#include <format>


bool EnablePanicMode = false;  // whether the parser is in panik

std::size_t                                  ScopeId{};
std::stack<Node*>                            ScopeTrack{};
std::vector<std::unique_ptr<Node>>           ParsedModule{};
std::unordered_map<std::string, std::size_t> SymbolTable{};  // maps the symbols to their scope


extern std::unordered_map<std::string, uint8_t> valid_expr_bin_ops;


std::unordered_map<std::string, int> precedence_table = {
        {"-",  1},
        {"+",  1},
        {"*",  2},
        {"/",  2},
        {"%",  2},
        {"**", 3},
        {">>", 4},
        {"<<", 4},
        {"&",  5},
        {"^",  6},
        {"|",  7},
        {"~",  8},
        {"<",  9},
        {"<=", 9},
        {">",  9},
        {">=", 9},
        {"==", 10},
        {"!=", 10}
};


void pushToModule(std::unique_ptr<Node> node, bool isNotParent = true) {
    if (!ScopeTrack.empty()) node->setParent(ScopeTrack.top());
    ParsedModule.emplace_back(std::move(node));
    if (!isNotParent) {
        ScopeTrack.push(ParsedModule.back().get());
    }
}


Parser::Parser(TokenStream &tks) : m_Stream(tks) {}
Parser::~Parser() = default;


void Parser::next(bool swsFlg, bool snsFlg) {
    cur_rd_tok = m_Stream.next(swsFlg, snsFlg);
}

void Parser::dispatch() {
    m_Stream.next();

    // uncomment to check the m_Stream's input for debugging
//    while (!m_Stream.eof()) {
//        m_Stream.next();
//    }


    while (!m_Stream.eof()) {
        TokenType t_type = m_Stream.p_CurTk.type;
        std::string t_val = m_Stream.p_CurTk.value;

        switch (t_type) {
            case KEYWORD:
                if (t_val == "var" || t_val == "const") {
                    parseVar();
                    continue;
                } else if (t_val == "fn") {
                    parseFunction();
                    continue;
                }
                break;

            case PUNC:
                if (t_val == "}") {
                    ScopeTrack.pop();
                    ScopeId--;
                } else if (t_val == "{")
                    ScopeId++;
                break;

            case IDENT:
                if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(") {
                    pushToModule(parseCall());
                }

                    // ignore rogue identifiers if they are valid
                else {
                    if (SymbolTable.find(t_val) == SymbolTable.end()) {
                        auto stream_state = m_Stream.getStreamState();
                        m_ExceptionHandler.newException(
                                ERROR,
                                stream_state.Line,
                                stream_state.Col - (t_val.size()),
                                stream_state.Col,
                                m_Stream.getLineFromSrc(stream_state.Line),
                                std::format("Undefined reference to the symbol {}", t_val)
                        );
                    }
                }
            break;
        }

        m_Stream.next();
    }

    m_ExceptionHandler.raiseAll();
    for (const auto& nd : ParsedModule) {
        nd->codegen();
    }
}


void Parser::parseFunction() {
    Function func_nd{};

    m_Stream.next();
    func_nd.ident = m_Stream.p_CurTk.value;

    // Check for errors
    if (SymbolTable.find(func_nd.ident) != SymbolTable.end()) {
        auto stream_state = m_Stream.getStreamState();
        EnablePanicMode = true;
        m_ExceptionHandler.newException(
                ERROR,
                stream_state.Line,
                stream_state.Col - (func_nd.ident.size()),
                stream_state.Col,
                stream_state.CurLn,
                "A function with this name already exists"
        );
    } else {
        SymbolTable[func_nd.ident] = ScopeId;
    }

    next(); next();
    ScopeId++;

    static auto parse_params = [this]() {
        decltype(func_nd.getParamInstance()) param{};
        param.var_ident = m_Stream.p_CurTk.value;  // parameter ident

        m_Stream.next(); m_Stream.next();
        param.var_type = m_Stream.p_CurTk.value;

        param.initialized = m_Stream.peek().type == PUNC && m_Stream.peek().value == "=";
        next();
        return param;
    };

    if (m_Stream.p_CurTk.type != PUNC) {
        while (m_Stream.p_CurTk.value != ")" && m_Stream.p_CurTk.type != PUNC) {
            func_nd.params.push_back(parse_params());
            if (m_Stream.p_CurTk.value == ",")
                next();
        }
    }

    // current token == ')'
    next();
    if (m_Stream.p_CurTk.type == IDENT)
        func_nd.ret_type = m_Stream.p_CurTk.value;

    pushToModule(std::make_unique<Function>(std::move(func_nd)), false);
}


void Parser::parseVar() {
    Var var_node;
    var_node.is_const = m_Stream.p_CurTk.value == "const";
    var_node.var_ident = m_Stream.next().value;

    if (m_Stream.peek().type == PUNC && m_Stream.peek().value == ":") {
        next();
        var_node.var_type = m_Stream.next().value;
    }

    if (m_Stream.peek().type == OP && m_Stream.peek().value == "=") {
        var_node.initialized = true;
        next();
        next();
        parseExpr(&var_node.value);
    }

    pushToModule(std::make_unique<Var>(std::move(var_node)));
}


std::unique_ptr<Node> Parser::parseCall() {
    std::unique_ptr<FuncCall> call_node = std::make_unique<FuncCall>();
    call_node->ident = m_Stream.p_CurTk.value;
    next();
    next();

    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")
        return call_node;

    while (m_Stream.p_CurTk.value != ")") {
        parseExpr(&call_node->args, true);
    }

    return call_node;
}


/* This method is an adaptation of the `Shunting Yard Algorithm`. */
void Parser::parseExpr(std::variant<std::vector<Expression>*, Expression*> ptr, bool isCall) {
    // TODO: remove overcomplexity

    bool kill_yourself = false;
    std::stack<Op> ops{};  // operator stack
    std::vector<std::unique_ptr<Node>> output{};  // the final postfix (RPN) form

    int paren_counter    = 0;
    int ops_opr_consumed = 0;

    Token prev_token;
    static const std::unordered_set<TokenType> invalid_prev_types{IDENT, NUMBER, KEYWORD, STRING};

    while (!m_Stream.eof()) {
        Op top_elem;

        // break once the expression ends
        if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ",") { m_Stream.next(); break; }
//        if ((m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")") && !invalid_prev_types.contains(prev_token.type)) break;
        if (m_Stream.p_CurTk.type == KEYWORD) break;
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
                    break;
                } output.emplace_back(std::make_unique<Ident>(Ident(m_Stream.p_CurTk.value)));
                    break;
            case OP:
                // pop ops and push them into output till the top operator of the stack has greater or equal precedence
                while (!ops.empty() && precedence_table[m_Stream.p_CurTk.value] <= precedence_table[ops.top().getValue()]) {
                    output.emplace_back(std::make_unique<Op>(ops.top()));
                    ops.pop();
                } ops.emplace(m_Stream.p_CurTk.value);
                  break;
            case PUNC:
                if (m_Stream.p_CurTk.value == "(") {
                    paren_counter++;
                    ops.emplace("(");
                    break;
                }
                else if (m_Stream.p_CurTk.value == ")") {
                    paren_counter--;
                    if (isCall && paren_counter == -1) { kill_yourself = true; break; }
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

        if (kill_yourself) { kill_yourself = false; break; }
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
    for (auto& nd : output) {
        auto n = std::make_unique<Node>();
        n = std::move(nd);
        expr.expr.push_back(std::move(n));
    }

    if (std::holds_alternative<std::vector<Expression>*>(ptr))
        std::get<std::vector<Expression>*>(ptr)->push_back(std::move(expr));
    else
        std::get<Expression*>(ptr)->expr = std::move(expr.expr);
}
