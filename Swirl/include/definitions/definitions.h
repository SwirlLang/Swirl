#include <iostream>
#include <array>

#ifndef SWIRL_DEFINITIONS_H
#define SWIRL_DEFINITIONS_H
struct defs {
    enum TokenType {
        STRING, IDENT, NUMBER, PUNC
    };

    std::array<const char *, 20> keywords = {
            "func", "return", "if", "else", "for", "while",
            "is", "in", "or", "and", "class", "public",
            "static", "int", "string", "float", "bool", "true",
            "false", "var"
    };

    std::array<std::string, 2> builtins = {
            "print", "len"
    };

    std::array<char, 9> op_chars = {'*', '!', '=', '%', '+', '-', '/', '>', '<'};

    std::array<char, 8> puncs = {'(', ')', ';', ',', '{', '}', '[', ']'};
    std::array<char, 6> delimiters = {'(', ')', ' ', '\n', ')', '{',};
};

#endif
