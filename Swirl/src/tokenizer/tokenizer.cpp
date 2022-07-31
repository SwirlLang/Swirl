#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>

#include <utils/utils.h>
#include <object/object.h>
//#include <transpiler/transpiler.h>

void transpile(std::string& expression, std::string fFilePath) {
    std::vector<std::string> builtins = {"print", "len", "complex"};
    std::string pr_txt = "#include <iostream>\nvoid print(const char* string, const char *end = \"\\n\") { std::cout << string << end; }\n";
    std::string file_name = fFilePath.substr(fFilePath.find_last_of("/\\") + 1);
    file_name = file_name.substr(0, file_name.find_last_of("."));
    std::ofstream w_cpp_obj_file(getWorkingDirectory(fFilePath) + PATH_SEP + "__swirl_cache__" + PATH_SEP + file_name + ".cpp");

    std::cout << expression << std::endl;
//    if (_expression.is_builtin) {
//        w_cpp_obj_file << pr_txt << std::endl;
//
//        w_cpp_obj_file << "int main() {";
//        w_cpp_obj_file << _expression.calls_to + '(';
//        for (auto const& arg : _expression.arguments) {
//            w_cpp_obj_file << arg;
//        }
//
//        w_cpp_obj_file << ");";
//        w_cpp_obj_file << "}";
//    }

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


void tokenize(std::string _source, std::string _fFilePath, bool _debug = false) {
    std::string c_byte;
    auto* tokens = new std::vector<std::string>();
    std::vector<std::string> expressions;
    std::array<char, 4> delimiters = {'(', ')', ' ', '\n'};

    // Makes tokens out of the char stream
    for (const auto& chr : _source) {
        if (std::find(delimiters.begin(), delimiters.end(), chr) != delimiters.end()) {
            tokens->push_back(c_byte);
            c_byte.clear();
            tokens->push_back(std::string(1, chr));
        } else {
            c_byte += chr;
        }
    }

    std::string exp;
    std::array<const char*, 2> expression{nullptr, nullptr};
    for (const auto elem : *tokens) {
        if (elem != "\n") {
            exp.append(elem);
        } else {
            transpile(exp, _fFilePath);
            exp.clear();
        }
    }

    for (auto elem : expressions) {
        LOG(elem)
    }

    delete tokens;
}