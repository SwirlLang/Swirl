#ifndef SWIRL_TOKENS_H
#define SWIRL_TOKENS_H

#include <string>

enum CompTimeTypes {
    CT_INT,
    CT_INT64,
    CT_INT128,

    CT_FLOAT,
    CT_DOUBLE,

    CT_BOOL,
    CT_UNDEDUCED,    // mark the type to be deduced
    CT_DO_NOT_CARE   // do not give a shit
};

enum TokenType {
    KEYWORD,
    IDENT,
    OP,
    STRING,
    PUNC,
    NUMBER,
    MACRO,
    COMMENT,

    NONE,
    _NONE // to be used in the parser, "" will be replaced with this
};

struct Token {
    TokenType type;
    std::string value;
    std::size_t at_line;

    CompTimeTypes data_type = CT_UNDEDUCED;
};

#endif //SWIRL_TOKENS_H
