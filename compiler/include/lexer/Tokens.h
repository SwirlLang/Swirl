#pragma once
#include <string>
#include <unordered_map>
#include <string_view>
#include <concepts>


enum TokenCategory {
    KEYWORD,
    IDENT,
    OP,
    STRING,
    CHAR,
    PUNC,
    NUMBER,
    COMMENT,
    NONE,
    UNINIT
};


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

    template <typename... Args>
    bool is(Args... args) {
        static_assert((std::same_as<Args, TokenValue> && ...));
        return ((args == tokenid) || ...);
    }

    static std::string_view toString(TokenValue v);
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


inline std::string_view Token::toString(TokenValue v) {
    switch (v) {
        case KW_RETURN: return "return";
        case KW_IF: return "if";
        case KW_ELSE: return "else";
        case KW_FOR: return "for";
        case KW_WHILE: return "while";
        case KW_MUT: return "mut";
        case KW_TRUE: return "true";
        case KW_FALSE: return "false";
        case KW_UNDEFINED: return "undefined";
        case KW_ENUM: return "enum";
        case KW_PROTOCOL: return "protocol";
        case KW_CONST: return "const";
        case KW_STATIC: return "static";
        case KW_BREAK: return "break";
        case KW_CONTINUE: return "continue";
        case KW_ELIF: return "elif";
        case KW_EXTERN: return "extern";
        case KW_COMPTIME: return "comptime";
        case KW_LET: return "let";
        case KW_IMPORT: return "import";
        case KW_EXPORT: return "export";
        case KW_VAR: return "var";
        case KW_FN: return "fn";
        case KW_VOLATILE: return "volatile";
        case KW_STRUCT: return "struct";
        case OP_AS: return "as";
        case OP_NOT: return "not";
        case OP_DOT: return ".";
        case OP_RANGE: return "..";
        case OP_MEMBER_ACCESS: return "->";
        case OP_SCOPE_RES: return "::";
        case OP_LOGICAL_OR: return "||";
        case OP_LOGICAL_AND: return "&&";
        case OP_NOT_EQ: return "!=";
        case OP_EQ: return "==";
        case OP_GT: return ">";
        case OP_LT: return "<";
        case OP_GT_EQ: return ">=";
        case OP_LT_EQ: return "<=";
        case OP_ASSIGN: return "=";
        case OP_PLUS: return "+";
        case OP_MINUS: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_EXP: return "**";
        case OP_EXP_ASSIGN: return "**=";
        case OP_PLUS_ASSIGN: return "+=";
        case OP_MINUS_ASSIGN: return "-=";
        case OP_MUL_ASSIGN: return "*=";
        case OP_DIV_ASSIGN: return "/=";
        case OP_MOD_ASSIGN: return "%=";
        case OP_ELLIPSIS: return "...";
        case OP_COMMENT: return "//";
        case OP_BITWISE_OR: return "|";
        case OP_BITWISE_AND: return "&";
        case OP_BITWISE_NOT: return "~";
        case OP_LBITSHIFT: return "<<";
        case OP_RBITSHIFT: return ">>";
        case OP_XOR: return "^";
        case OP_BITWISE_OR_ASSIGN: return "|=";
        case OP_BITWISE_AND_ASSIGN: return "&=";
        case OP_BITWISE_NOT_ASSIGN: return "~=";
        case OP_LBITSHIFT_ASSIGN: return "<<=";
        case OP_RBITSHIFT_ASSIGN: return ">>=";
        case OP_XOR_ASSIGN: return "^=";
        case PUNC_SEMI: return ";";
        case PUNC_COMMA: return ",";
        case PUNC_LPAREN: return "(";
        case PUNC_RPAREN: return ")";
        case PUNC_LBRACE: return "{";
        case PUNC_RBRACE: return "}";
        case PUNC_LBRACKET: return "[";
        case PUNC_RBRACKET: return "]";
        case PUNC_COLON: return ":";
        case PUNC_HASH: return "#";
        case NUM_INT: return "INTEGER";
        case NUM_FLOAT: return "FLOAT";
        case IDENT: return "IDENT";
        case STRING: return "STRING";
        default: return "unknown";
    }
}