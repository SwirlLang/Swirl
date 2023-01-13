#include <iostream>
#include <array>

#ifndef SWIRL_DEFINITIONS_H
#define SWIRL_DEFINITIONS_H

struct defs {
    std::array<const char *, 27> keywords = {
            "func", "return", "if", "else", "for", "while",
            "is", "in", "or", "and", "class", "public",
            "private", "int", "string", "float", "bool",
            "true", "false", "var", "const", "static", "break",
            "continue", "elif", "global", "importc"
    };

    std::array<char, 9> op_chars = {'*', '!', '=', '%', '+', '-', '/', '>', '<'};

    std::array<char, 8> puncs = {'(', ')', ';', ',', '{', '}', '[', ']'};
    std::array<char, 6> delimiters = {'(', ')', ' ', '\n', ')', '{',};
};

#endif
