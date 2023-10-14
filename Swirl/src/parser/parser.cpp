#include <cmath>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include <parser/parser.h>

#define n_ptr std::make_unique<Node>

using namespace std::string_literals;

// Flags
short ang_ind = 0;
uint8_t is_first_t = 1;
uint8_t rd_param = 0;
uint8_t rd_func = 0;
uint8_t rd_param_cnt = 0;

std::vector<Node> Module{};
extern std::unordered_map<std::string, const char *> type_registry;
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
        {"~",  8}
};

// this function is meant to be used for debugging purpose
void handleNodes(NodeType type, std::unique_ptr<Node>& nd) {
    switch (type) {
        case ND_IDENT:
            std::cout << nd->getValue() << " ";
            break;
        case ND_FLOAT:
            break;
        case ND_INT:
            std::cout << nd->getValue() << " ";
            break;
        case ND_OP:
            std::cout << nd->getValue() << " ";
            break;
        default:
            break;
    }
}


Parser::Parser(TokenStream &tks) : m_Stream(tks) {}
Parser::~Parser() = default;

void Parser::next(bool swsFlg, bool snsFlg) {
    cur_rd_tok = m_Stream.next(swsFlg, snsFlg);
}

void appendModule(const Node &nd) {
//    Module.emplace(nd);
}

void Parser::dispatch() {
    int br_ind = 0;
    int prn_ind = 0;
    const char *tmp_ident = "";
    const char *tmp_type = "";

    m_Stream.next();

    while (!m_Stream.eof()) {
        TokenType t_type = m_Stream.p_CurTk.type;
        std::string t_val = m_Stream.p_CurTk.value;

        if (t_type == KEYWORD) {
            if (t_val == "var" || t_val == "const") {
                parseVar();
                continue;
            }
        }

        if (t_type == IDENT) {
            if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(") {
                appendModule(*parseCall());
            }
        }
        m_Stream.next();
    }
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
        parseExpr();
    }
}


std::unique_ptr<FuncCall> Parser::parseCall() {
    std::cout << "call" << std::endl;
    std::unique_ptr<FuncCall> call_node = std::make_unique<FuncCall>();
    call_node->ident = m_Stream.p_CurTk.value;
    next();
    next();

    if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")
        return call_node;

    parseExpr( call_node->ident);

    while (true) {
//        if (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")) m_Stream.next();
        if (m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")
            break;
        if (m_Stream.p_CurTk.value == "," && m_Stream.p_CurTk.type == PUNC) {
            m_Stream.next();
            parseExpr(call_node->ident);
        }
    }

//    parseArgs();
    m_Stream.next();
    return call_node;
}

/* This method converts the expression into a postfix form, each expression object it creates
 * consists of two vectors, one for the operands, the other for the operators sorted
 * in the order of their precedence by this algorithm. Inspired from the Shunting-Yard-Algorithm.
 * The method assumes that the current token(m_Stream.p_CurTk) is the start of the expression.*/
void Parser::parseExpr(const std::string id) {
    std::stack<Op> ops{};  // our operator stack
    std::vector<std::unique_ptr<Node>> output{};  // the final postfix(RPN) form

    int paren_counter    = 0;
    int ops_opr_consumed = 0;

    Token prev_token;
    static const std::unordered_set<TokenType> invalid_prev_types{IDENT, NUMBER, KEYWORD, STRING};

    while (!m_Stream.eof()) {
        Op top_elem;

        // break once the expression ends
        if (m_Stream.p_CurTk.type == KEYWORD) { break; }
        if (ops_opr_consumed > 1) {
            if (m_Stream.p_CurTk.type == KEYWORD) { break; }

            if ((invalid_prev_types.contains(prev_token.type) && m_Stream.p_CurTk.type != OP)) {
                if (!(m_Stream.p_CurTk.type == PUNC && m_Stream.p_CurTk.value == ")")) { break; }
            }
        }

        std::cout << "in expr -> " << m_Stream.p_CurTk.value << std::endl;
        switch (m_Stream.p_CurTk.type) {
            case NUMBER:
                output.emplace_back(std::make_unique<Int>(Int(m_Stream.p_CurTk.value)));
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

        ops_opr_consumed++;
        prev_token = m_Stream.p_CurTk;
        m_Stream.next();
    }

    while (!ops.empty()) {
        output.emplace_back(std::make_unique<Op>(ops.top()));
        ops.pop();
    }

//     uncomment for debugging purpose
    for (auto& elem : output) {
        handleNodes(elem->getType(), elem);
    } std::cout << " <--- in postfix" <<  "\n";
}
