#include <iostream>
#include <array>

#ifndef SWIRL_DEFINITIONS_H
#define SWIRL_DEFINITIONS_H
struct defs {
    enum TokenType {
        Keyword, Id, Integer, String, Float, Bool,
        Parentheses, Operator, Brace, Punctuation
    };

    std::array<const char *, 20> keywords = {
            "func", "return", "if", "else", "for", "while",
            "is", "in", "or", "and", "class", "public",
            "static", "int", "string", "float", "bool", "true",
            "false", "var"
    };

    std::array<std::string, 5> builtins = {
            "print", "len", "abs", "hash", "eval"
    };

    std::array<char, 9> op_chars = {'*', '!', '=', '%', '+', '-', '/', '>', '<'};

    std::array<char, 6> delimiters = {'(', ')', ' ', '\n', ')', '{',};
};

#endif
