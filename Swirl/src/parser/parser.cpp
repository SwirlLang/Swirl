#include <iostream>
#include <cstring>
#include <array>
#include <string>
#include <memory>
#include <vector>

#include <unordered_map>
#include <parser/parser.h>
#include <exception/exception.h>


using namespace std::string_literals;

// States
int        br_ind          =  0;
int        prn_ind         =  0;
short      ang_ind         =  0;
bool       rd_call         =  false;
bool       rd_param        =  false;
bool       rd_func         =  false;
bool       rd_expr         =  false;
uint8_t    rd_param_cnt    =  false;


std::vector<Node*> call_tracks{};
std::vector<Node*> scope_tracks{};

extern std::unordered_map<std::string, const char* > type_registry;


template <typename T>
typename std::enable_if<std::is_base_of<Node, T>::value>::type
reinitialize(T& inst) {
    inst = T();
}

void Parser::appendAST(Node& node, bool pushTo) {
    m_AST->chl.push_back(node);
}

Parser::Parser(TokenStream& _stream) : m_Stream(_stream) {
    m_AST = new AbstractSyntaxTree{};
}

Parser::~Parser() {
    delete m_AST;
}

void Parser::next(bool swsFlg, bool snsFlg) {
    cur_rd_tok = m_Stream.next(swsFlg, snsFlg);

    if (cur_rd_tok.type == PUNC)
        if (cur_rd_tok.value == " " || cur_rd_tok.value == "\n")
            next();
}

void Parser::dispatch() {
    const char* tmp_ident = "";
    const char* tmp_type  = "";

    Node tmp_node{};

    next();
    while (cur_rd_tok.type != NONE) {
        TokenType t_type = cur_rd_tok.type;
        std::string t_val(cur_rd_tok.value);


        std::cout << "val: " << t_val << '\n';
        if (t_type == IDENT) {
            if (type_registry.contains(t_val)) {
                if (m_Stream.peek().type == IDENT) {
                    parseVars();
                }
            }

            if (m_Stream.peek().type == PUNC && m_Stream.peek().value == "(") {
                parseCall();
            }
        }

        if (t_type == PUNC) {
            if (t_val == "(")
                prn_ind++;
            if (t_val == ")")
                prn_ind--;
        }
        next();
    }
}

void Parser::parseVars() {
    NdAssignment var{};
    var.type = cur_rd_tok.value;

    next();
    var.ident = cur_rd_tok.value;
    std::cout << var.ident << " -T- " << var.type << std::endl;

    if (m_Stream.peek().value == "=" && m_Stream.peek().type == PUNC)
        var.initialized = true;
    else
        appendAST(var);
}

void Parser::parseCall() {
    prn_ind++;
}
