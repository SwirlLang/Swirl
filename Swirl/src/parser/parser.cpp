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

    Node tmp_node{};

    try {
        cur_rd_tok = m_Stream.next();

        while (strcmp(cur_rd_tok[0], "null") != 0) {
            std::string t_type(cur_rd_tok[0]);
            std::string t_val(cur_rd_tok[1]);

            if (t_type == "PUNC") {
                if (t_val == "(") {tmp_node.type = "PRN_OPEN";}
                else if (t_val == ")") {tmp_node.type = "PRN_CLOSE";}
                else if (t_val == ",") {tmp_node.type = "COMMA";}
                else if (t_val == "{") {tmp_node.type = "BR_OPEN";}
                else if (t_val == "}") {tmp_node.type = "BR_CLOSE";}
                else if (t_val == ":") {tmp_node.type = "COLON";}
                else if (t_val == ".") {tmp_node.type = "DOT"; }
                else {cur_rd_tok = m_Stream.next(); continue;}
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                cur_rd_tok = m_Stream.next();
                continue;
            }

            if (t_type == "KEYWORD") {
                if (t_val == "if" || t_val == "elif" || t_val == "else") {
                    parseCondition(t_val.c_str());
                    cur_rd_tok = m_Stream.p_CurTk;
                    continue;
                } else if (t_val == "while" || t_val == "for") {
                    parseLoop(t_val.c_str());
                    cur_rd_tok = m_Stream.p_CurTk;
                    continue;
                } else if (t_val == "func") {
                    parseFunction();
                    continue;
                }

                tmp_node.type = "KEYWORD";
                tmp_node.value = t_val;
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                cur_rd_tok = m_Stream.next();
                continue;
            }

            if (t_type == "IDENT") {
                tmp_ident = t_val.c_str();
                if (strcmp(tmp_type, "") != 0) {
                    parseDecl(tmp_type, tmp_ident);
                    tmp_type = "";
                    tmp_ident = "";
                    cur_rd_tok = m_Stream.next();
                    continue;
                }

                cur_rd_tok = m_Stream.next();
                if (strcmp(m_Stream.p_CurTk[1], "(") == 0) {
                    parseCall(tmp_ident);
                    continue;
                }
                tmp_node.type = "IDENT";
                tmp_node.value = tmp_ident;
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                continue;
            }

            if (t_type == "OP" || t_type == "NUMBER" || t_type == "STRING") {
                tmp_node.type = t_type;
                tmp_node.value = t_val;
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                cur_rd_tok = m_Stream.next();
                continue;
            }

            if (t_type == "MACRO") {
                tmp_node.type = t_type;
                tmp_node.value = t_val;
                m_AST->chl.push_back(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                cur_rd_tok = m_Stream.next();
                continue;
            }

            for (const std::string& tp : registered_types)
                if (tp == t_val)
                    tmp_type = tp.c_str();

            cur_rd_tok = m_Stream.next();
        }
    } catch ( std::exception& _ ) {}
}

void Parser::parseFunction() {
    int br_ind;
    Node func_node{};
    func_node.type = "FUNCTION";
    func_node.ctx_type = "void";
    cur_rd_tok = m_Stream.next();
    func_node.ident = cur_rd_tok[1];

    cur_rd_tok = m_Stream.next();
    m_AST->chl.push_back(func_node);
}

void Parser::parseDecl(const char* _type, const char* _ident) {
    Node decl_node{};
    decl_node.type = "VAR";
    decl_node.ctx_type = _type;
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

//    m_Stream.next();
    if (!m_AppendToScope) m_AST->chl.push_back(call_node);
}

void Parser::parseLoop(const char* _type) {
    Node loop_node{};
    loop_node.type = _type;

    try {
        cur_rd_tok = m_Stream.next();
        while (strcmp(m_Stream.p_CurTk[0], "null") != 0) {
            if (strcmp(m_Stream.p_CurTk[0], "PUNC") == 0 && strcmp(m_Stream.p_CurTk[1], "{") == 0)
                break;
            loop_node.value += strcat((char*)m_Stream.p_CurTk[1], " ");
            cur_rd_tok = m_Stream.next();
        }
    } catch ( std::exception& _sigabrt ) { }
    m_AST->chl.push_back(loop_node);
}

void Parser::parseCondition(const char* _type) {
    Node cnd_node{};
    cnd_node.type = _type;

    try {
        cur_rd_tok = m_Stream.next();
        while (::strcmp(m_Stream.p_CurTk[0], "null") != 0) {
            if (strcmp(m_Stream.p_CurTk[0], "PUNC") == 0 && strcmp(m_Stream.p_CurTk[1], "{") == 0)
                break;

            cnd_node.value += strcat((char*)m_Stream.p_CurTk[1], " ");
            cur_rd_tok = m_Stream.next();
        }
    } catch ( std::exception& _sigabrt ) { }

    m_AST->chl.push_back(cnd_node);
}