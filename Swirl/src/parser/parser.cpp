#include <iostream>
#include <cstring>
#include <array>

#include <parser/parser.h>
#include <exception/exception.h>


Parser::Parser(TokenStream& _stream) : m_Stream(_stream) {
    m_AST = new AbstractSyntaxTree{};
}

Parser::~Parser() {
    delete m_AST;
}

void Parser::dispatch() {
    const char* tmp_ident = "";
    const char* tmp_type = "";
    auto _debug = 1;

    Node tmp_node{};

    try {
        while (!m_Stream.eof()) {
            std::string t_val(m_Stream.p_CurTk[1]);
            std::string t_type(m_Stream.p_CurTk[0]);

            if (t_type == "PUNC" && t_val == "(") {
                tmp_node.type = "PRN_OPEN";
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
            }

            if (t_type == "PUNC" && t_val == ")") {
                tmp_node.type = "PRN_CLOSE";
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
            }

            if (t_type == "KEYWORD" && t_val == "if" || t_val == "elif" || t_val == "else") {
                parseCondition(t_val.c_str());
                continue;
            }

            if (t_type == "IDENT" && strcmp(tmp_type, "") != 0) {
                tmp_ident = t_val.c_str();
                parseDecl(tmp_type, tmp_ident);
                tmp_type = "";
                tmp_ident = "";
                m_Stream.next();
                continue;
            }

            if (t_type == "IDENT") {
                tmp_ident = t_val.c_str();

                m_Stream.next();
                if (strcmp(m_Stream.p_CurTk[1], "(") == 0) {
                    parseCall(tmp_ident);
                    continue;
                }

                tmp_node.type = "IDENT";
                tmp_node.value = tmp_ident;
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                m_Stream.next();
                continue;
            }

            if (t_type == "KEYWORD" && t_val == "while" || t_val == "for") {
                parseLoop(t_val.c_str());
                continue;
            }

            if (t_type == "NUMBER" || t_type == "STRING") {
                tmp_node.type = t_type;
                tmp_node.value = t_val;
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                m_Stream.next();
                continue;
            }

            if (t_type == "OP") {
                tmp_node.type = t_type;
                tmp_node.value = t_val;
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                m_Stream.next();
                continue;
            }

            if (t_type == "KEYWORD" || t_type == "IDENT")
                for (const std::string& tp : registered_types)
                    if (tp == t_val)
                        tmp_type = tp.c_str();

            m_Stream.next();
        }
    } catch ( std::exception& _ ) {}
}

void Parser::parseDecl(const char* _type, const char* _ident) {
    Node decl_node{};
    decl_node.type = "VAR";
    decl_node.var_type = _type;
    decl_node.ident = _ident;

    try {
        m_Stream.next();
        if (strcmp(m_Stream.p_CurTk[0], "OP") == 0 && strcmp(m_Stream.p_CurTk[1], "=") == 0)
            decl_node.initialized = true;
    } catch ( std::exception& _ ) {}

    m_AST->chl.push_back(decl_node);
}

void Parser::parseCall(const char* _ident) {
    Node call_node{};
    Node arg_node{};
    call_node.type = "CALL";
    call_node.ident = _ident;
    call_node.scope_order = m_ScopeId;

    m_Stream.next();
    if (!m_AppendToScope) m_AST->chl.push_back(call_node);
}

void Parser::parseLoop(const char* _type) {
    Node loop_node{};
    loop_node.type = _type;

    m_Stream.next();
    while (strcmp(m_Stream.p_CurTk[0], "PUNC") != 0 && strcmp(m_Stream.p_CurTk[1], "{") != 0)
        loop_node.condition += m_Stream.p_CurTk[1];

    m_AST->chl.push_back(loop_node);
    std::cout << loop_node.condition << std::endl;
}

void Parser::parseCondition(const char* _type) {
    Node cnd_node{};
    cnd_node.type = _type;

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
