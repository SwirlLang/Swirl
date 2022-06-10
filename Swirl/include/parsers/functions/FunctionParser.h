#include <sstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>

#include <utils/utils.h>

#ifndef Swirl_FUNCTION_PARSER_H
#define Swirl_FUNCTION_PARSER_H
#define FunctionParserFlags FunctionParserFlags

enum FunctionParserFlags
{
    DIRECT_STRING,
    NONE
};

template <typename Indexes>
auto parseFunctions(const char *_funcString, bool &_funcParserExitSuccess, Indexes _arrayIndexes,
                    Indexes _listIndexes, FunctionParserFlags flags = FunctionParserFlags::NONE)
{
};

#endif
