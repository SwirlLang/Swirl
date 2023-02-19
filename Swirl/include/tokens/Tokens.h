#ifndef SWIRL_TOKENS_H
#define SWIRL_TOKENS_H

enum TokenType {
    KEYWORD,
    IDENT,
    OP,
    STRING,
    PUNC,
    NUMBER,
    MACRO,

    NONE,

    // Extra Tokens required by the parser

    TYPEDEF,
    EXPORT,
    FUNCTION,
    FOR,
    WHILE,
    COLON,
    IMPORT,
    PRN_OPEN,
    PRN_CLOSE,
    COMMA,
    BR_OPEN,
    BR_CLOSE,
    IF,
    ELIF,
    ELSE,
    DOT,
    VAR,
    CALL,

    _NONE, // to be used in the parser, "" will be replaced with this
};

struct Token {
    TokenType type;
    const char *value;
};

#endif //SWIRL_TOKENS_H
