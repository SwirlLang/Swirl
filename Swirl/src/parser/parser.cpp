#include <iostream>
#include <cstring>
#include <array>

#include <unordered_map>
#include <parser/parser.h>
#include <exception/exception.h>


short      ang_ind      = 0;
uint8_t    rd_param     = 0;
uint8_t    rd_func      = 0;
uint8_t    rd_param_cnt = 0;

extern std::unordered_map<std::string, const char* > type_registry;

void Parser::appendAST(Node& node) {
    auto state = m_Stream.getStreamState();
    node.loc["col"] = state["col"];
    node.loc["line"] = state["line"];
    
    if (rd_param) m_AST->chl.back().arg_nodes.push_back(node);
    else if (ang_ind > 0) {
        m_AST->chl.back().template_args.push_back(node);
        if (node.type == "IDENT")
            type_registry[node.value] = "template";
    }
    else if (!rd_param && rd_func)  m_AST->chl.back().body.push_back(node);
    else { m_AST->chl.push_back(node); }
}

Parser::Parser(TokenStream& _stream) : m_Stream(_stream) {
    m_AST = new AbstractSyntaxTree{};
}

Parser::~Parser() {
    delete m_AST;
}

void Parser::next() { cur_rd_tok = m_Stream.next(); }

void Parser::dispatch() {
    int         br_ind    = 0;
    int         prn_ind   = 0;
    const char* tmp_ident = "";
    const char* tmp_type  = "";

    Node tmp_node{};

    cur_rd_tok = m_Stream.next();

    while (strcmp(cur_rd_tok[0], "null") != 0) {
        std::string t_type(cur_rd_tok[0]);
        std::string t_val(cur_rd_tok[1]);

        if (t_type == "PUNC") {
            if (rd_func) {
                if (t_val == "(" && !rd_param_cnt) { ++prn_ind; rd_param = true;}
                if (t_val == ")" && !rd_param_cnt) {
                    prn_ind--;
                    if (!prn_ind) {
                        tmp_node.type = "PRN_CLOSE";
                        appendAST(tmp_node);
                        tmp_node.type = "";
                        rd_param = false;
                        rd_param_cnt++;
                        cur_rd_tok = m_Stream.next();
                        if (strcmp(cur_rd_tok[0], "PUNC") == 0 && strcmp(cur_rd_tok[1], ":") == 0) {
                            cur_rd_tok = m_Stream.next();
                            m_AST->chl.back().ctx_type = cur_rd_tok[1];
                            cur_rd_tok = m_Stream.next();
                            continue;
                        } continue;
                    }
                }

                if (t_val == "{") { br_ind++; }
                else if (t_val == "}") {
                    br_ind--;
                    if (br_ind == 0) {
                        rd_func = false;
                        tmp_node.type = "BR_CLOSE";
                        m_AST->chl.back().body.push_back(tmp_node);
                        tmp_node.type = "";
                        rd_param = false;
                        rd_param_cnt = 0;
                        cur_rd_tok = m_Stream.next();
                        continue;
                    }
                }
            }

            if (t_val == "(") {tmp_node.type = "PRN_OPEN";}
            else if (t_val == ")") {tmp_node.type = "PRN_CLOSE";}
            else if (t_val == ",") {tmp_node.type = "COMMA";}
            else if (t_val == "{") {tmp_node.type = "BR_OPEN";}
            else if (t_val == "}") {tmp_node.type = "BR_CLOSE";}
            else if (t_val == ":") {tmp_node.type = "COLON";}
            else if (t_val == ".") {tmp_node.type = "DOT"; }
            else {cur_rd_tok = m_Stream.next(); continue;}
            appendAST(tmp_node);
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
                rd_func = true;
                continue;
            } else if (t_val == "from") {
                tmp_node.type = "IMPORT";

                while (strcmp(m_Stream.next()[1], "import") != 0)
                    tmp_node.from += m_Stream.p_CurTk[1];

                while (strcmp(m_Stream.next(true)[1], "\n") != 0)
                    tmp_node.impr += m_Stream.p_CurTk[1];

                appendAST(tmp_node);

                tmp_node.type = tmp_node.from = tmp_node.impr = "";
                cur_rd_tok = m_Stream.next();
                continue;
            } else if (t_val == "export") {
                tmp_node.type = "EXPORT";
                while (strcmp(m_Stream.next(true)[1], "\n") != 0)
                    if (strcmp(m_Stream.p_CurTk[0] , "IDENT") == 0)
                        tmp_node.body.push_back(Node{.value=m_Stream.p_CurTk[1]});
                appendAST(tmp_node);
                tmp_node.type = "";
                cur_rd_tok = m_Stream.next();
                continue;
            } else if (t_val == "typedef") {
                tmp_node.type = "TYPEDEF";
                tmp_node.ident = m_Stream.next()[1];

                type_registry[m_Stream.p_CurTk[1]] = "";
                while (strcmp(m_Stream.next(true, true)[1], "\n") != 0)
                    tmp_node.value += m_Stream.p_CurTk[1];

                appendAST(tmp_node);
                tmp_node.type = "";
                tmp_node.value = "";
                tmp_node.ident = "";
                cur_rd_tok = m_Stream.next();
                continue;
            }

            tmp_node.type = "KEYWORD";
            tmp_node.value = t_val;
            appendAST(tmp_node);
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
            appendAST(tmp_node);
            tmp_node.type = "";
            tmp_node.value = "";
            continue;
        }

        if (t_type == "STRING") {
            if (t_val.starts_with("f")) {
                tmp_node.format = true;
                t_val.erase(0, 1);
            }

            tmp_node.type = t_type;
            tmp_node.value = t_val;
            appendAST(tmp_node);

            tmp_node.format = false;
            tmp_node.type = "";
            tmp_node.value = "";
            cur_rd_tok = m_Stream.next();
            continue;
        }

        if (t_type == "OP") {
            if (t_val == "<" && !rd_param && rd_func) { ang_ind++; next(); continue; }
            if (t_val == ">" && !rd_param && rd_func) { ang_ind--; next(); continue; }

            if (t_val == "//") {
                while (strcmp(m_Stream.next(true)[1], "\n") != 0 || strcmp(m_Stream.next(true)[1], "null") != 0)
                    m_Stream.next();
                cur_rd_tok = m_Stream.next();
                continue;
            }

            tmp_node.type = t_type;
            tmp_node.value = tmp_node.ident = t_val;
            appendAST(tmp_node);
            tmp_node.type = tmp_node.value = tmp_node.type = "";
            cur_rd_tok = m_Stream.next();
            continue;
        }

        if (t_type == "NUMBER") {
            tmp_node.type = t_type;
            tmp_node.value = t_val;
            appendAST(tmp_node);
            tmp_node.type = "";
            tmp_node.value = "";
            cur_rd_tok = m_Stream.next();
            continue;
        }

        if (t_type == "MACRO") {
            tmp_node.type = t_type;
            tmp_node.value = t_val;
            appendAST(tmp_node);
            tmp_node.type = "";
            tmp_node.value = "";
            cur_rd_tok = m_Stream.next();
            continue;
        }
        cur_rd_tok = m_Stream.next();
    }
}

void Parser::parseFunction() {
    int br_ind;
    Node func_node{};
    func_node.type = "FUNCTION";
    func_node.ctx_type = "auto";
    cur_rd_tok = m_Stream.next();
    func_node.ident = cur_rd_tok[1];

    cur_rd_tok = m_Stream.next();
    appendAST(func_node);
}

void Parser::parseDecl(const char* _type, const char* _ident) {
    Node decl_node{};
    decl_node.type = "VAR";
    decl_node.ctx_type = _type;
    decl_node.ident = _ident;

    try {
        cur_rd_tok = m_Stream.next();
        if (strcmp(cur_rd_tok[0], "OP") == 0 && strcmp(cur_rd_tok[1], "=") == 0)
            decl_node.initialized = true;
    } catch ( std::exception& _ ) {}

    appendAST(decl_node);
}

void Parser::parseCall(const char* _ident) {
    Node call_node{};
    Node arg_node{};
    call_node.type = "CALL";
    call_node.ident = _ident;

//    m_Stream.next();
//    if (m_AST->chl[-1].type == "FUNCTION" && m_AST->chl[-1].body[-1].ident == _ident)
//        m_AST->chl[-1].body.pop_back();
    appendAST(call_node);
}

void Parser::parseLoop(const char* _type) {
    Node loop_node{};
    loop_node.type = _type;

    cur_rd_tok = m_Stream.next();
    while (strcmp(m_Stream.p_CurTk[0], "null") != 0) {
        if (strcmp(m_Stream.p_CurTk[0], "PUNC") == 0 && strcmp(m_Stream.p_CurTk[1], "{") == 0)
            break;
        loop_node.value += strcat((char*)m_Stream.p_CurTk[1], " ");
        cur_rd_tok = m_Stream.next();
    }

    appendAST(loop_node);
}

void Parser::parseCondition(const char* _type) {
    Node cnd_node{};
    cnd_node.type = _type;

    cur_rd_tok = m_Stream.next();
    while (::strcmp(m_Stream.p_CurTk[0], "null") != 0) {
        if (strcmp(m_Stream.p_CurTk[0], "PUNC") == 0 && strcmp(m_Stream.p_CurTk[1], "{") == 0)
            break;

        cnd_node.value += strcat((char*)m_Stream.p_CurTk[1], " ");
        cur_rd_tok = m_Stream.next();
    }

    appendAST(cnd_node);
}
