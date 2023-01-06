#include <iostream>

#include <parser/parser.h>

#define SC_IF_IN_PRNS if (!prn_ind) compiled_source += ";"

std::string compiled_source = R"(
#include <iostream>
#include <vector>

#define float double
#define elif else if
#define var auto
#define __COLON__ ->
#define in :

template < typename Obj >
void print(Obj __Obj, const std::string& __End = "\n", bool __Flush = true) {
    if (__Flush) std::cout << std::boolalpha << __Obj << __End << std::flush;
    else std::cout << std::boolalpha << __Obj << __End;
}

std::string input(const std::string& __Prompt) {
    std::string ret;
    std::cout << __Prompt << std::flush;
    std::getline(std::cin, ret);

    return ret;
}

std::vector<int> range(int __begin, int __end = 0) {
    // TODO: use an input iterator
    std::vector<int> ret{};
    if (!__end) { __end = __begin; __begin = 0; }
    for (int i = __begin; i < __end; i++)
        ret.emplace_back(i);
    return ret;
}
)";

void Transpile(AbstractSyntaxTree& _ast, const std::string& _buildFile) {
    int                        prn_ind = 0;
    int                        fn_br_ind = 0;
    int8_t                     rd_function = false;

    std::ifstream              bt_fstream{};
    std::string                tmp_str_cnst{};
    std::string                last_node_type{};
    std::string                macros{};

    std::size_t bt_size = compiled_source.size();
    compiled_source += "int main() {\n";

    for (auto const& child : _ast.chl) {
        if (child.type == "OP") {
            if (!prn_ind)
                compiled_source.erase(compiled_source.size() - 1);
            compiled_source += child.value;

            if (child.value == "++" || child.value == "--")
                compiled_source += ";";
            continue;
        }

        if (child.type == "FUNCTION") {
            rd_function = true;
            compiled_source += "auto " + child.ident + " = " + "[]";
            continue;
        }

        if (child.type == "for" || child.type == "while") {
            compiled_source += child.type + " (" + child.value + ")";
            continue;
        }

        if (child.type == "COLON") {
            compiled_source += " __COLON__ ";
            continue;
        }

        if (child.type == "KEYWORD") {
            if   (child.value == "break" || child.value == "continue")
                 { compiled_source += child.value + ";"; continue; }
            else { compiled_source += child.value + " "; continue; }
        }

        if (child.type == "MACRO") {
            macros += "#" + child.value + "\n";
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

            if (rd_function && !prn_ind) rd_function = -1;
            if (!prn_ind) {
                if (rd_function != -1) { compiled_source += ";"; continue; }
                else rd_function = 0;
            }
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
            fn_br_ind += 1;
            if (compiled_source[compiled_source.size() - 2] == ')')
                compiled_source.erase(compiled_source.size() - 1);
            compiled_source += "{";
            continue;
        }

        if (child.type == "BR_CLOSE") {
            fn_br_ind -= 1;
            compiled_source += "}";

            if (!fn_br_ind) compiled_source += ';';
            if (!fn_br_ind ) { }
            continue;
        }

        if (child.type == "if" || child.type == "elif" || child.type == "else") {
            if (child.type == "else")
                compiled_source += "else";
            else
                compiled_source += child.type + " (" + child.value + ")";
            continue;
        }

        if (child.type == "DOT") {
            if (compiled_source.ends_with(';'))
                compiled_source.erase(compiled_source.size() - 1);
            compiled_source += ".";
            continue;
        }

        if (child.type == "VAR") {
            compiled_source += child.ctx_type + " " + child.ident;
            if (!child.initialized)
                compiled_source += ";";
            else
                compiled_source += "=";
            continue;
        }

        if (child.type == "CALL")
            compiled_source += child.ident;
        last_node_type = child.type;
    }

    compiled_source.insert(bt_size, macros);
    std::ofstream o_file_buf(_buildFile);
    compiled_source += "}";
    o_file_buf << compiled_source;
    o_file_buf.close();
}
