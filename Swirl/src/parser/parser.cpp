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
    std::string tmp_ident;
    try {
        while (!m_Stream.eof()) {
            std::string t_val(m_Stream.p_CurTk[1]);
            std::string t_type(m_Stream.p_CurTk[0]);
            if (t_type == "IDENT") {
                tmp_ident = t_val;
                m_Stream.next();
                if (std::string(m_Stream.p_CurTk[1]) == "(" ) {
                    parseCall(tmp_ident.c_str());
                }
            }
            m_Stream.next();
        }
    } catch ( std::exception& _sigbrt ) {}
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
//                decl_node.value = m_Stream.p_CurTk;
                break;
            }
            m_Stream.next();
        }
    } catch ( std::exception& _sigbrt ) { std::cout << _sigbrt.what(); }
//    decl_node.value = m_Stream.p_CurTk;
    if (!m_AppendToScope) m_AST->chl.push_back(decl_node);
}

void Parser::parseCall(const char* _ident) {
    Node call_node{};
    Node arg_node{};
    call_node.type = "CALL";
    call_node.ident = _ident;

    while (strcmp(m_Stream.next()[1], ")") != 0) {
        const auto tk_cache = m_Stream.p_CurTk;
        const std::string tk_type(tk_cache[0]);
        const std::string tk_val(tk_cache[1]);

//        std::cout << m_Stream.p_CurTk[0] << std::endl;
        if (tk_type != "PUNC") {  // TODO: scenario not considered: f(<object>.<method>())
//            std::cout << "REACHED!" << std::endl;
            arg_node.type = tk_type;
            arg_node.value = tk_val;
            call_node.args.push_back(arg_node);
        }
    }

    if (!m_AppendToScope) m_AST->chl.push_back(call_node);
}

void Parser::parseCondition() {
    Node cnd_node{};
    cnd_node.type = "CONDITION";
    std::string t_type(m_Stream.p_CurTk[0]);
    std::string t_val(m_Stream.p_CurTk[1]);
    std::array<const char *, 4> valid_cmp_tk_type = { "IDENT", "NUMBER", "BOOL", "STRING" };

    try {
        while (!m_Stream.eof()) {
            if (std::find(valid_cmp_tk_type.begin(), valid_cmp_tk_type.end(), m_Stream.p_CurTk[0]) != valid_cmp_tk_type.end()) {
                std::cout << "comparisons done in: " << m_Stream.p_CurTk[1] << std::endl;
            }

            m_Stream.next();
            if (strcmp(m_Stream.p_CurTk[0], "PUNC") != 0) {
                std::cout << "op: " << m_Stream.p_CurTk[1] << std::endl;
            }
        }
    } catch ( std::exception& _sigbrt ) { }
}
