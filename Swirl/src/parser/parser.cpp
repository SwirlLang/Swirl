#include <cmath>
#include <deque>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include <parser/parser.h>
#include <tokens/Tokens.h>
#include <unordered_map>
#include <variant>

using namespace std::string_literals;

// Flags
short ang_ind = 0;
uint8_t is_first_t = 1;
uint8_t rd_param = 0;
uint8_t rd_func = 0;
uint8_t rd_param_cnt = 0;

std::vector<Node> Module{};
extern std::unordered_map<std::string, const char *> type_registry;
extern std::unordered_map<std::string, uint8_t> non_assign_binary_ops;

std::unordered_map<std::string, uint8_t> default_operator_precedence = {
        {"+",  0},
        {"-",  1},
        {"*",  2},
        {"/",  3},
        {"**", 4},
        {">>", 5},
        {"<<", 6},
        {"^",  7},
        {"|",  8},
        {"&",  9},
        {"~",  10}
};

Parser::Parser(TokenStream &tks) : m_Stream(tks) {}

Parser::~Parser() = default;

void Parser::next(bool swsFlg, bool snsFlg) {
    cur_rd_tok = m_Stream.next(swsFlg, snsFlg);
}

void appendModule(const Node &nd) {
//    Module.emplace_back(nd);
}

void traverse(std::shared_ptr<Expression> &expr) {
    for (auto elem: expr->evaluation_ord) {
        if (elem->getType() == ND_EXPR)
            traverse(elem);
    }
}

void Parser::dispatch() {
    int br_ind = 0;
    int prn_ind = 0;
    const char *tmp_ident = "";
    const char *tmp_type = "";

    m_Stream.next();
    while (m_Stream.p_CurTk.type != NONE) {
        TokenType t_type = m_Stream.p_CurTk.type;
        std::string t_val = m_Stream.p_CurTk.value;

        if (t_type == KEYWORD) {
            if (t_val == "var" || t_val == "const") {
                parseVar();
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
    std::cout << "var" << std::endl;
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

//    std::function<void()> parseArgs = [this, &call_node, &parseArgs]() -> void {
//        if (m_Stream.p_CurTk.value == "," && m_Stream.p_CurTk.type == PUNC) {
//            m_Stream.next();
//            parseExpr(&call_node->args, call_node->ident);
//        }
//        parseArgs();
//    };

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


void Parser::parseExpr(const std::string id) {
    std::vector<std::shared_ptr<Expression>> expr{};
    std::vector<std::shared_ptr<Expression>> current_expr_grp_ptr{};

    int paren_cnt = 0;
    bool give_up_control = false;
    
    auto push_to_expr = [&expr, &current_expr_grp_ptr](const std::shared_ptr<Expression>& node) -> void {
        if (current_expr_grp_ptr.empty()) { expr.emplace_back(node); }
        else { current_expr_grp_ptr.back()->evaluation_ord.emplace_back(node); }
    };

    std::cout << "parsing expression... for " << id << std::endl;
    while (!m_Stream.eof()) {
        std::cout << "val: " << m_Stream.p_CurTk.value << " type: " << m_Stream.p_CurTk.type << std::endl;
        if (give_up_control) { give_up_control = false; break; }
        if (m_Stream.p_CurTk.type == OP) {
           if (!non_assign_binary_ops.contains(m_Stream.p_CurTk.value)) {
               give_up_control = true;
           }
        }

        if (m_Stream.p_CurTk.type == IDENT && m_Stream.peek().type == PUNC && m_Stream.peek().value == "(")
            push_to_expr(parseCall());

        switch (m_Stream.p_CurTk.type) {
            case STRING:
                push_to_expr(std::make_shared<String>(m_Stream.p_CurTk.value));
                std::cout << "E: " << m_Stream.p_CurTk.value << std::endl;
                m_Stream.next();
                continue;

            case PUNC:
                if (m_Stream.p_CurTk.value == "(") {
                    paren_cnt++;
                    auto expr_grp = std::make_shared<Expression>();
                    push_to_expr(expr_grp);
                    current_expr_grp_ptr.emplace_back(expr_grp);
                    m_Stream.next();
                    continue;
                }

                if (m_Stream.p_CurTk.value == ")") {
                    if (paren_cnt == 0) { return; }
                    else { paren_cnt--; };
                    std::cout << "C" << std::endl;
                    current_expr_grp_ptr.pop_back();
                    m_Stream.next();
                    continue;
                }


            case NUMBER:
                std::cout << "num: " << m_Stream.p_CurTk.value << std::endl;

                try {
                    if (m_Stream.p_CurTk.value.find('.') != std::string::npos)
                        push_to_expr(std::make_shared<Double>(std::stod(m_Stream.p_CurTk.value)));
                    else
                        push_to_expr(std::make_shared<Int>(std::stoi(m_Stream.p_CurTk.value)));
                    m_Stream.next();
                } catch (const std::invalid_argument& _) { return; }

                continue;

            case IDENT:
                push_to_expr(std::make_shared<Ident>(m_Stream.p_CurTk.value));
                m_Stream.next();
                continue;

            case OP:
                if (non_assign_binary_ops.contains(m_Stream.p_CurTk.value)) {
                    push_to_expr(std::make_shared<Op>(m_Stream.p_CurTk.value));
                    std::cout << "OP: " << m_Stream.p_CurTk.value << std::endl;
                    m_Stream.next();
                } else break;
        }
    }
}
