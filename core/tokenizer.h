#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <regex>

#include "utils/utils.h"

#ifndef TOKENIZER_H_LAMBDA_CODE
#define TOKENIZER_H_LAMBDA_CODE

template <typename Any>
std::vector<std::string> tokenize(const char* _str, Any _stringIndices, bool& _tokenizerExitCode) {
    std::vector<std::string> ret;

    std::regex class_pattern("class[\s+]+[]");

    std::regex_search cls_matches(_str, class_pattern);

    _tokenizerExitCode = true;
    return ret;
}

#endif