#include <iostream>
#include <exception>

#include <parser/parser.h>

std::string compiled_source = R"(
#include <iostream>

#define elif else if
#define var auto

template < typename Const >
void print(Const __Obj, const std::string& __End = "\n", bool __Flush = true) {
    if (__Flush) std::cout << std::boolalpha << __Obj << __End << std::flush;
    else std::cout << std::boolalpha << __Obj << __End;
}

std::string input(std::string __Prompt) {
    std::string ret;
    std::cout << __Prompt << std::flush;
    std::getline(std::cin, ret);

    return ret;
}
)";

void Transpile(AbstractSyntaxTree& _ast, const std::string& _buildFile) {
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

        if (child.type == "STRING") {
            compiled_source += '"' + child.value + "\";";
            continue;
        }

        if (child.type == "NUMBER") {
            compiled_source += child.value + ";";
            continue;
        }

        if (child.type == "IDENT") {
            compiled_source += child.value + ";";
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
            tmp_str_cnst += child.ident + "(";
            int args_count = 0;

            for (const auto& arg : child.args) {
                if (arg.type == "STRING")
                    tmp_str_cnst += "\"" + arg.value + "\"";
                else
                    tmp_str_cnst += arg.value;

                args_count++;
                if (args_count != child.args.size()) tmp_str_cnst += ",";
            }

            tmp_str_cnst += ")";
            compiled_source += tmp_str_cnst + ";";
            tmp_str_cnst.clear();
        }
    }

    std::ofstream o_file_buf(_buildFile);
    compiled_source += "}";
    o_file_buf << compiled_source;
    o_file_buf.close();
}
