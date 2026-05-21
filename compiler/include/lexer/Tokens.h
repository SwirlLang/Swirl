#pragma once
#include <string>
#include <unordered_map>


enum TokenCategory {
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


inline const char*to_string(const TokenCategory e) {
    switch (e) {
        case KEYWORD:
            return "KEYWORD";
        case IDENT:
            return "IDENT";
        case OP:
            return "OP";
        case STRING:
            return "STRING";
        case PUNC:
            return "PUNC";
        case NUMBER:
            return "NUMBER";
        case COMMENT:
            return "COMMENT";
        case NONE:
            return "NONE";
        case UNINIT:
            return "UNINIT";
        default:
            return "unknown";
    }
}


struct StreamState {
    std::size_t Line{}, Pos{}, Col{};
};


struct Token {
    enum TokenValue {
        KW_RETURN,
        KW_IF,
        KW_ELSE,
        KW_FOR,
        KW_WHILE,
        KW_MUT,
        KW_TRUE,
        KW_FALSE,
        KW_UNDEFINED,
        KW_ENUM,
        KW_PROTOCOL,
        KW_CONST,
        KW_STATIC,
        KW_BREAK,
        KW_CONTINUE,
        KW_ELIF,
        KW_EXTERN,
        KW_COMPTIME,
        KW_LET,
        KW_IMPORT,
        KW_EXPORT,
        KW_VAR,
        KW_FN,
        KW_VOLATILE,
        KW_STRUCT,

        OP_AS,
        OP_NOT,
        OP_DOT,
        OP_RANGE,
        OP_MEMBER_ACCESS,
        OP_SCOPE_RES,
        OP_LOGICAL_OR,
        OP_LOGICAL_AND,
        OP_NOT_EQ,
        OP_EQ,
        OP_GT,
        OP_LT,
        OP_GT_EQ,
        OP_LT_EQ,
        OP_ASSIGN,
        OP_PLUS,
        OP_MINUS,
        OP_MUL,
        OP_DIV,
        OP_MOD,
        OP_EXP,
        OP_EXP_ASSIGN,
        OP_PLUS_ASSIGN,
        OP_MINUS_ASSIGN,
        OP_MUL_ASSIGN,
        OP_DIV_ASSIGN,
        OP_MOD_ASSIGN,
        OP_ELLIPSIS,
        OP_COMMENT,

        OP_BITWISE_OR,
        OP_BITWISE_AND,
        OP_BITWISE_NOT,
        OP_LBITSHIFT,
        OP_RBITSHIFT,
        OP_XOR,

        OP_BITWISE_OR_ASSIGN,
        OP_BITWISE_AND_ASSIGN,
        OP_BITWISE_NOT_ASSIGN,
        OP_LBITSHIFT_ASSIGN,
        OP_RBITSHIFT_ASSIGN,
        OP_XOR_ASSIGN,

        PUNC_SEMI,
        PUNC_COMMA,
        PUNC_LPAREN,
        PUNC_RPAREN,
        PUNC_LBRACE,
        PUNC_RBRACE,
        PUNC_LBRACKET,
        PUNC_RBRACKET,
        PUNC_COLON,
        PUNC_HASH,

        NUM_INT,
        NUM_FLOAT,

        IDENT,

        STRING,
    };

    TokenCategory type{};
    std::string value;
    StreamState location{};

    TokenValue tokenid{};

    bool operator==(const Token& other) const {
        return type == other.type && value == other.value;
    }
};


inline const
std::unordered_map<std::string_view, Token::TokenValue>
KeywordMap = {
    {"return", Token::KW_RETURN},
    {"if", Token::KW_IF},
    {"else", Token::KW_ELSE},
    {"for", Token::KW_FOR},
    {"while", Token::KW_WHILE},
    {"mut", Token::KW_MUT},
    {"true", Token::KW_TRUE},
    {"false", Token::KW_FALSE},
    {"undefined", Token::KW_UNDEFINED},
    {"enum", Token::KW_ENUM},
    {"protocol", Token::KW_PROTOCOL},
    {"const", Token::KW_CONST},
    {"static", Token::KW_STATIC},
    {"break", Token::KW_BREAK},
    {"continue", Token::KW_CONTINUE},
    {"elif", Token::KW_ELIF},
    {"extern", Token::KW_EXTERN},
    {"comptime", Token::KW_COMPTIME},
    {"let", Token::KW_LET},
    {"import", Token::KW_IMPORT},
    {"export", Token::KW_EXPORT},
    {"var", Token::KW_VAR},
    {"fn", Token::KW_FN},
    {"volatile", Token::KW_VOLATILE},
    {"struct", Token::KW_STRUCT}
};
