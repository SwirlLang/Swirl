#pragma once
#include <string>

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

inline const char* to_string(const TokenType e) {
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

enum TokenMeta {
    CT_INT,
    CT_FLOAT,
};

struct StreamState {
    std::size_t Line{}, Pos{}, Col{};
};


struct Token {
    TokenType type{};
    std::string value;
    StreamState location;

    TokenMeta meta{};

    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }
};
