#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#ifndef UTILS_H_LAMBDA_CODE
#define UTILS_H_LAMBDA_CODE

    template<typename Any>
    bool _checkMembership(Any, char*);

    std::vector<std::string> _splitString(std::string&, char);

#endif