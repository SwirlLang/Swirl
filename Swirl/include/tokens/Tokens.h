#ifndef SWIRL_TOKENS_H
#define SWIRL_TOKENS_H

#include <string>

struct StreamState {
    std::size_t Line, Pos, Col;
};

enum CompTimeTypes {
    CT_FLOAT,
    CT_INT,
    CT_BOOL,
};

enum TokenType {
    KEYWORD, // 0
    IDENT, // 1
    OP, // 2
    STRING, // 3
    PUNC, // 4
    NUMBER, // 5
    COMMENT, // 6

    NONE, // 7
    UNINIT // 8
};

inline const char* to_string(TokenType e) {
    switch (e) {
        case KEYWORD: return "KEYWORD";
        case IDENT: return "IDENT";
        case OP: return "OP";
        case STRING: return "STRING";
        case PUNC: return "PUNC";
        case NUMBER: return "NUMBER";
        case COMMENT: return "COMMENT";
        case NONE: return "NONE";
        case UNINIT: return "UNINIT";
        default: return "unknown";
    }
}

struct Token {
    TokenType type;
    std::string value;
    StreamState location;

    CompTimeTypes data_type{};
};

#endif //SWIRL_TOKENS_H
