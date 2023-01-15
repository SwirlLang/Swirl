#include <variant>
#include <random>
#include <parser/parser.h>

#define SC_IF_IN_PRNS if (!prn_ind) _dest += ";"

std::string compiled_funcs;
std::string compiled_source = R"(
#include <iostream>
#include <vector>

#define elif else if
#define var auto
#define __COLON__ ->
#define in :

using string = std::string;

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

std::size_t bt_size = compiled_source.size();

std::string format(std::string& str) {
    bool escape = false;
    std::string ret;

    for (char chr : str) {
        if (chr == '{' && !escape)
            ret += "\"+";
        else if (chr == '}' && !escape)
            ret += "+\"";
        else if (chr == '\\')
            escape = true;
        else { ret += chr; if (escape) escape = false; };
    } return ret;
}

void Transpile(std::vector<Node>& _nodes, const std::string& _buildFile,
                std::string& _dest = compiled_source, bool onlyAppend = false) {
    unsigned int     prn_ind       = 0;
    int              fn_br_ind     = 0;
    int              rd_function   = 0;
    bool             read_ret_type = false;
    bool             is_include    = false;

    std::ifstream    bt_fstream{};
    std::string      tmp_str_cnst{};
    std::string      last_node_type{};
    std::string      last_func_ident{};
    std::string      cimports{};
    std::string      macros{};

    if (_dest == compiled_source)
        _dest += "int main() {\n";

    for (Node& child : _nodes) {

        if (child.type == "OP") {
            if (!prn_ind)
                _dest.erase(_dest.size() - 1);
            _dest += child.value;
            if (child.value == "++" || child.value == "--")
                _dest += ";";
            continue;
        }

        if (child.type == "STRING") {
            if (is_include) { cimports += child.value + "\n"; is_include = false; continue; }
            else if (child.format) { _dest += "string(" + format(child.value) + ")"; SC_IF_IN_PRNS; continue; }
            _dest += "string(" + child.value + ")";
            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "FUNCTION") {
            rd_function = true;
            last_func_ident = child.ident;
            compiled_funcs += child.ctx_type + " " + child.ident;

            Transpile(child.arg_nodes, _buildFile, compiled_funcs, true);
            Transpile(child.body, _buildFile, compiled_funcs, true);
            continue;
        }

        if (child.type == "for" || child.type == "while") {
            _dest += child.type + " (" + child.value + ")";
            continue;
        }

        if (child.type == "COLON") {
            _dest += " __COLON__ ";
            read_ret_type = true;
            continue;
        }

        if (child.type == "KEYWORD") {
            if   (child.value == "break" || child.value == "continue")
                { _dest += child.value + ";"; continue; }
            else if (child.value == "true" || child.value == "false")
                { _dest += child.value; SC_IF_IN_PRNS; continue;}
            else if (child.value == "importc")
                { cimports += "#include "; is_include = true; continue;}
            else{ _dest += child.value + " "; continue; }
        }

        if (child.type == "MACRO") {
            macros += "#" + child.value + "\n";
            continue;
        }

        if (child.type == "PRN_OPEN") {
            _dest += "(";
            prn_ind++;
            continue;
        }

        if (child.type == "PRN_CLOSE") {
            _dest += ")";
            prn_ind--;

            if (rd_function && !prn_ind) rd_function = -1;
            if (!prn_ind) {
                if (rd_function != -1) { _dest += ";"; continue; }
                else rd_function = 0;
            }
            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "COMMA") {
            _dest += ",";
            continue;
        }

        if (child.type == "NUMBER") {
            _dest += child.value;
            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "IDENT") {
            if (read_ret_type) {
                read_ret_type = false;
                _dest.replace(_dest.find(last_func_ident) - 5, 4, child.value);
            }
            _dest += child.value;
            SC_IF_IN_PRNS;
            continue;
        }

        if (child.type == "BR_OPEN") {
            fn_br_ind ++;
            if (_dest[_dest.size() - 2] == ')')
                _dest.erase(_dest.size() - 1);
            _dest += "{";
            continue;
        }

        if (child.type == "BR_CLOSE") {
            fn_br_ind --;
            _dest += "}";

            if (!fn_br_ind) _dest += ';';
            if (!fn_br_ind ) { }
            continue;
        }

        if (child.type == "if" || child.type == "elif" || child.type == "else") {
            if (child.type == "else")
                { if (_dest[_dest.size() - 1] == ';') _dest.erase(_dest.size() - 1); _dest += "else"; }
            else
                _dest += child.type + " (" + child.value + ")";
            continue;
        }

        if (child.type == "DOT") {
            if (_dest.ends_with(';')) _dest.erase(_dest.size() - 1);
            _dest += ".";
            continue;
        }

        if (child.type == "VAR") {
            _dest += child.ctx_type + " " + child.ident;
            if (!child.initialized && !rd_function) _dest += ";";
            else _dest += "=";
            continue;
        }

        if (child.type == "CALL")
            _dest += child.ident;
        last_node_type = child.type;
    }

    if (!onlyAppend) {
        _dest.insert(bt_size, compiled_funcs);
        _dest.insert(0, cimports);  // this inserts all your importc statements to the top of the file

        _dest.insert(0, "\n" + macros + "\n");

        std::ofstream o_file_buf(_buildFile);
        _dest += "}";
        o_file_buf << _dest;
        o_file_buf.close();
    }
}
