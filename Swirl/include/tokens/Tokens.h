#ifndef SWIRL_TOKENS_H
#define SWIRL_TOKENS_H

#include <string>


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
};

#endif //SWIRL_TOKENS_H
