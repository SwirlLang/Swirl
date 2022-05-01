#include <iostream>
#include <string>
#include <cstring>

#include "../../utils/utils.h"
#include "../functions/FunctionParser.h"

#ifndef CLASS_PARSER_H_Swirl
#define CLASS_PARSER_H_Swirl

template <typename Any>
ClassParserRet parseClass(const char *_classString, std::vector<Any> _dictIndexes,
                          bool &_classParserExitCode)
{
    std::string cls_cppStr(_classString);
    std::stringstream cls_cppStr_stream(cls_cppStr);
    std::string current_ln;

    ClassParserRet ret{};

    while (std::getline(cls_cppStr_stream, current_ln))
    {
        if (current_ln.find("class") != std::string::npos)
        {
            ret.identifier = splitString(current_ln, '(').erase(0, current_ln.find("class") + 5);
            std::string params = splitString(current_ln, ')').erase(0, current_ln.find('(') + 1);
            FunctionParserRet constructor{};
            ret.constructors.push_back(constructor.params = params);
            ret.params = params;
            if (ret.identifier == "" || ret.identifier == " ")
            {
                _classParserExitCode = false;
                return ret;
            }
        }

        if (current_ln.find(ret.identifier) != std::string::npos)
        {
            FunctionParserRet secondaryConstructor{};
            std::string params = splitString(current_ln, ')').erase(0, current_ln.find('(') + 1);
            ret.constructors.push_back(secondaryConstructor.params = params);
        }
    }

    _classParserExitCode = true;
    return ret;
}
#endif