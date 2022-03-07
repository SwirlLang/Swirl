#include <sstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>

#include "../../utils/utils.h"

#ifndef LAMBDA_CODE_FUNCTION_PARSER_H
#define LAMBDA_CODE_FUNCTION_PARSER_H
#define FunctionParserFlags FunctionParserFlags

enum FunctionParserFlags {
    DIRECT_STRING, NONE
};

template <typename Indexes>
auto parseFunctions(const char* _funcString, bool& _funcParserExitSuccess, Indexes _arrayIndexes,
Indexes _listIndexes, FunctionParserFlags flags = FunctionParserFlags::NONE) {
    /* The parser is very basic and minimal for now, more things will be added soon */

    // TODO make the parser check brace presence in arrays
    
    std::stringstream func_string_stream(_funcString);
    std::string current_line;
    std::string final_ret_info;

    while (std::getline(func_string_stream, current_line)) {
        if (current_line.find("func") != std::string::npos) {
            std::string func_name = splitString(current_line, '(').erase(0, current_line.find("func") + 4);
            final_ret_info += "\n name: " + func_name;
            std::string param_f_cpy;

            if (current_line.find('{') != std::string::npos) {
                std::string h_params = splitString(splitString(current_line, '{'), ')');
                std::string params_str = h_params.erase(0, h_params.find("func") + h_params.find("(") + 1);
                final_ret_info += "\n params: " + params_str;
                auto temp = current_line.find(*strchr(current_line.c_str(), ')'));
            } else {
                std::string h_params = splitString(current_line, ')');
                std::string params_str = h_params.erase(0, h_params.find("func") + h_params.find("(") + 1);
                final_ret_info += "\n params: " + params_str;
            }
        }
    }

    _funcParserExitSuccess = true;
    return final_ret_info;
}

#endif
