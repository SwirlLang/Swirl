#include <iostream>
#include <exception>

#include <parser/parser.h>

#define SC_IF_IN_PRNS if (!prn_ind) compiled_source += ";"

std::string compiled_source = R"(
#include <iostream>

#define elif else if
#define var auto

template < typename Const >
void print(Const __Obj, const std::string& __End = "\n", bool __Flush = true) {
    if (__Flush) std::cout << std::boolalpha << __Obj << __End << std::flush;
    else std::cout << std::boolalpha << __Obj << __End;
}

std::string input(const std::string& __Prompt) {
    std::string ret;
    std::cout << __Prompt << std::flush;
    std::getline(std::cin, ret);

    return ret;
}
)";

void Transpile(AbstractSyntaxTree& _ast, const std::string& _buildFile) {
    int                        prn_ind = 0;
    bool                       is_scp;
    bool                       apnd_chl;
    std::ifstream              bt_fstream;
    std::string                tmp_str_cnst;
    std::size_t                last_scp_order;

    compiled_source += "int main() {\n";

    for (auto const& child : _ast.chl) {
        if (child.type == "OP") {
            compiled_source.erase(compiled_source.size() - 1);
            compiled_source += child.value;

            if (child.value == "++" || child.value == "--")
                compiled_source += ";";
            continue;
        }

        if (child.type == "PRN_OPEN") {
            compiled_source += "(";
            prn_ind += 1;
            continue;
        }

        if (child.type == "PRN_CLOSE") {
            compiled_source += ")";
            prn_ind -= 1;

            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "STRING") {
            compiled_source += '"' + child.value + "\"";
            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "COMMA") {
            compiled_source += ",";
            continue;
        }

        if (child.type == "NUMBER") {
            compiled_source += child.value;
            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "IDENT") {
            compiled_source += child.value;
            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "BR_OPEN") {
            compiled_source += "{";
            continue;
        }

        if (child.type == "BR_CLOSE") {
            compiled_source += "}";
            continue;
        }

        if (child.type == "if" || child.type == "elif" || child.type == "else") {
            if (child.type == "else")
                tmp_str_cnst += "else";
            else
                tmp_str_cnst += child.type + " (" + child.condition + ")";
            compiled_source += tmp_str_cnst;
            tmp_str_cnst.clear();
            continue;
        }

        if (child.type == "VAR") {
            compiled_source += child.var_type + " " + child.ident;
            if (!child.initialized)
                compiled_source += ";";
            else {
                apnd_chl = true;
                compiled_source += "=";
            }
            continue;
        }

        if (child.type == "CALL") {
            compiled_source += child.ident + "(";
            prn_ind += 1;
        }
    }

    std::ofstream o_file_buf(_buildFile);
    compiled_source += "}";
    o_file_buf << compiled_source;
    o_file_buf.close();
}
