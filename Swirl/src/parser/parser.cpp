#include <iostream>
#include <cstring>
#include <array>

#include <parser/parser.h>
#include <exception/exception.h>


#define _debug true;

Parser::Parser(TokenStream& _stream) : m_Stream(_stream) {
    m_AST = new AbstractSyntaxTree{};
}

Parser::~Parser() {
    delete m_AST;
}

std::array<const char*, 2> Parser::next(std::array<const char *, 2>& _lvalue) {
    if (strcmp(_lvalue[0], "PUNC") != 0 && strcmp(_lvalue[1], " ") != 0) {
        _lvalue = m_Stream.next();
    }
}

void Parser::dispatch() {
//    try {
//        while (!m_Stream.eof()) {
//            std::string t_val(m_Stream.p_CurTk[1]);
//            std::string t_type(m_Stream.p_CurTk[0]);
//            const char* tmp_ident;
//
//            if (t_type == "IDENT") {
//                tmp_ident = m_Stream.p_CurTk[1];
//                if (m_Stream.next() == std::array<const char*, 2> {"PUNC", "("}) {
//                    parseCall(tmp_ident);
//                }
//            }
//        }
//    } catch ( std::exception& _sigbrt ) {}
}

void Parser::parseDecl() {
    Node decl_node{};
    decl_node.type = "VAR";

    std::array<const char*, 4> s_types = {"STRING", "NUMBER"};
    try {
        while (strcmp(m_Stream.p_CurTk[0], "IDENT") != 0) m_Stream.next();
    } catch ( std::exception& _sigbrt) {}
    decl_node.ident =  m_Stream.p_CurTk[1];
    try {
        while (true) {
            std::string decl_val;
            if (m_Stream.p_CurTk[0] == "NUMBER" || m_Stream.p_CurTk[0] == "STRING") {
                decl_val = m_Stream.p_CurTk[1];
                decl_node.value = m_Stream.p_CurTk;
                break;
            }
            m_Stream.next();
        }
    } catch ( std::exception& _sigbrt ) { std::cout << _sigbrt.what(); }
    decl_node.value = m_Stream.p_CurTk;
    if (!m_AppendToScope) m_AST->chl.push_back(decl_node);
}

void Parser::parseCall(const char* _ident) {
    Node call_node{};
    Node arg_node{};
    call_node.type = "CALL";
//    call_node.value = _ident, call_node.ident = _ident;

    while (strcmp(m_Stream.next()[1], ")") != 0) {
        const auto tk_cache = m_Stream.p_CurTk;
        const std::string tk_type(tk_cache[0]);
        const std::string tk_val(tk_cache[1]);

//        std::cout << m_Stream.p_CurTk[0] << std::endl;
        if (tk_type != "PUNC" && tk_val != _ident) {
//            std::cout << "REACHED!" << std::endl;
            arg_node.type = tk_type;
//            arg_node.value = tk_val;
            call_node.args.push_back(arg_node);
        }
    }
    if (!m_AppendToScope) m_AST->chl.push_back(call_node);
}

void Parser::parseCondition() {
    const char *cmp1 = nullptr;
    const char *cmp_opr = nullptr;
    const char *cmp2 = nullptr;

    Node cnd_node{};
    cnd_node.type = "CONDITION";
    std::array<const char *, 4> valid_cmp_tokens = { "IDENT", "NUMBER", "BOOL", "STRING" };

    bool n_end_scp;
    std::size_t cnd_parse_index = 0;
    std::array<const char *, 2> cur_tok = m_Stream.next();
    next(cur_tok);

    while (strcmp(cur_tok[1], "{") != 0) {
        if (strcmp(cur_tok[1], " ") != 0) {
            if (strcmp(cur_tok[0], "IDENT") != 0) {
//                if (m_Stream.peekChar() == '(')
//                    parseCall(cur_tok[1]);
            }
        }
        cur_tok = m_Stream.next();
    }
    std::cout << cmp1 << std::endl;
}
