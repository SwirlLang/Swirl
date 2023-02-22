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

    // Name of the Token Groups packaged by the Parser

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
    std::string_view value;
};

#endif //SWIRL_TOKENS_H
