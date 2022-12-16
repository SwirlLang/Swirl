#include <iostream>
#include <cstring>
#include <array>
#include <ranges>

#include <parser/parser.h>
#include <exception/exception.h>


#define _debug true;

Parser::Parser(TokenStream& _stream) : m_Stream(_stream) {
    m_AST = new AbstractSyntaxTree{};
}

Parser::~Parser() {
    delete m_AST;
}

void Parser::dispatch() {
    const char* tmp_ident = "";
    const char* tmp_type = "";
    Node brace_node_instance{};

    try {
        while (!m_Stream.eof()) {
            std::string t_val(m_Stream.p_CurTk[1]);
            std::string t_type(m_Stream.p_CurTk[0]);

            if (strcmp(m_Stream.p_CurTk[0], "PUNC") == 0 && strcmp(m_Stream.p_CurTk[1], "{") == 0) {
                m_ScopeId++;
                m_AST->chl.back().scope_order = m_ScopeId ;
                brace_node_instance.type = "BR_OPEN";
                m_AST->chl.push_back(brace_node_instance);
                brace_node_instance.type = "";
            }

            if (strcmp(m_Stream.p_CurTk[0], "PUNC") == 0 && strcmp(m_Stream.p_CurTk[1], "}") == 0) {
                m_ScopeId--;
                brace_node_instance.type = "BR_CLOSE";
                m_AST->chl.push_back(brace_node_instance);
                brace_node_instance.type = "";
            }

            if (t_type == "IDENT" && strcmp(tmp_type, "") != 0) {
                tmp_ident = t_val.c_str();
                parseDecl(tmp_type, tmp_ident);
                tmp_type = "";
                tmp_ident = "";
            }

            if (t_type == "KEYWORD" && t_val == "if") {
                parseCondition();
                continue;
            }

            if (t_type == "IDENT") {
                tmp_ident = t_val.c_str();
                m_Stream.next();
                if (std::string(m_Stream.p_CurTk[1]) == "(" ) {
                    parseCall(tmp_ident);
                }
            }

            if (t_type == "KEYWORD" || t_type == "IDENT")
                for (const std::string& tp : registered_types)
                    if (tp == t_val)
                        tmp_type = tp.c_str();

            m_Stream.next();
        }
    } catch ( std::exception& _sigabrt ) {}
}

void Parser::parseDecl(const char* _type, const char* _ident) {
    Node decl_node{};
    decl_node.type = "VAR";
    decl_node.type = _type;
    decl_node.ident = _ident;

    std::cout << "TYPE: " << _type << " IDENT: " << _ident << std::endl;

}

void Parser::parseCall(const char* _ident) {
    Node call_node{};
    Node arg_node{};
    call_node.type = "CALL";
    call_node.ident = _ident;
    call_node.scope_order = m_ScopeId;

    while (strcmp(m_Stream.next()[1], ")") != 0) {
        const auto tk_cache = m_Stream.p_CurTk;
        const std::string tk_type(tk_cache[0]);
        const std::string tk_val(tk_cache[1]);
        std::string tmp_ident;

        if (tk_type == "IDENT") {
            tmp_ident = tk_val;
        }
//        std::cout << m_Stream.p_CurTk[0] << std::endl;
        if (tk_type != "PUNC") {  // TODO: recursive checks for calls
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

    try {
        m_Stream.next();
        while (!m_Stream.eof()) {
            if (strcmp(m_Stream.p_CurTk[0], "PUNC") == 0 && strcmp(m_Stream.p_CurTk[1], "{") == 0)
                break;
            cnd_node.condition += strcat((char*)m_Stream.p_CurTk[1], " ");
            m_Stream.next();
        }
    } catch ( std::exception& _sigabrt ) { }

    m_AST->chl.push_back(cnd_node);
}

