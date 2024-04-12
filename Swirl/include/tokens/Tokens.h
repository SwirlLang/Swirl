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

    NONE,
    _NONE // to be used in the parser, "" will be replaced with this
};

struct Token {
    TokenType type;
    std::string value;
};

#endif //SWIRL_TOKENS_H
