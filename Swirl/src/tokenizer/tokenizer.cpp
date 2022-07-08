#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include <utils/utils.h>
#include <object/object.h>
//#include <transpiler/transpiler.h>

void transpile(Object _expression, const std::string& fFilePath ) {
    std::vector<std::string> builtins = {"print", "len", "complex"};
    std::string pr_txt = "#include <iostream>\nvoid print(const char* string, const char *end = \"\\n\") { std::cout << string << end; }\n";
    std::string file_name = fFilePath.substr(fFilePath.find_last_of("/\\") + 1);
    file_name = file_name.substr(0, file_name.find_last_of("."));
    std::ofstream w_cpp_obj_file(getWorkingDirectory(fFilePath) + getPathSep() + "__swirl_cache__" + getPathSep() + file_name + ".cpp");

    if (_expression.is_builtin) {
        w_cpp_obj_file << pr_txt << std::endl;

        w_cpp_obj_file << "int main() {";
        w_cpp_obj_file << _expression.calls_to + '(';
        for (auto const& arg : _expression.arguments) {
            w_cpp_obj_file << arg;
        }

        w_cpp_obj_file << ");";
        w_cpp_obj_file << "}";
    }

    w_cpp_obj_file.close();
}

template <class Type, typename T>
std::size_t getIndex(std::vector<Type>& _vec, T _item) {
    std::size_t index = 0;
    for (auto const& item : _vec) {
        if (item == _item)
            return index;
        index += 1;
    }
    return -1;
}


void tokenize(std::string _source, std::string _fFilePath) {
    std::vector<std::string> builtins = {
            "print", "len", "complex", "input"
    };

    auto* tokens = new std::vector<std::string>();
    std::string cur_byte;
    std::vector<std::string> delimiters =
            { "=", "+", "-", "*","/", "%", "&&", "!","//",
              "///", "(", ")",".", "{", "}", "class", "func",
              "import", "\n", " ", ":"
            };

    Object expression{};
    std::string s_current_ln;
    auto ss_buf = std::istringstream(_source);

    while ( std::getline(ss_buf, s_current_ln)) {
        // TODO implement Multiline expression support
        for (auto const& builtin : builtins) {
            if (s_current_ln.find(builtin) != std::string::npos) {
                expression.is_builtin = true;
                expression.calls_to = builtin;

                s_current_ln.erase(s_current_ln.find(builtin), builtin.size());
                s_current_ln.erase(s_current_ln.find('('), s_current_ln.find('(') + 1);
                s_current_ln.erase(s_current_ln.find(')'), s_current_ln.find(')') + 1);

                for (const auto& arg : splitIntoIterable(s_current_ln, ',')) {
                    expression.arguments.push_back(arg);
                }
            }
        }

        transpile(expression, _fFilePath);
    }
}