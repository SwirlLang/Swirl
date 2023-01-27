#include <iostream>
#include <array>

#ifndef SWIRL_DEFINITIONS_H
#define SWIRL_DEFINITIONS_H

struct defs {
    std::array<std::string, 27> keywords = {
            "func", "return", "if", "else", "for", "while",
            "is", "in", "or", "and", "class", "public",
            "private" "true", "false", "var", "const", "static", "break",
            "continue", "elif", "global", "importc", "typedef",
            "import", "export", "from"
    };

    std::array<char, 9> op_chars = {'*', '!', '=', '%', '+', '-', '/', '>', '<'};

    std::array<char, 8> puncs = {'(', ')', ';', ',', '{', '}', '[', ']'};
    std::array<char, 6> delimiters = {'(', ')', ' ', '\n', ')', '{',};
};

#endif
