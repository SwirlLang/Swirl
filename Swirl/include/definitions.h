#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>


// /// maps {previous-node's type, current node's type} to the corresponding deducted type
// inline const std::unordered_map<std::pair<Type::SwTypes, Type::SwTypes>, Type*> DeductionGuide =
// {
//     {{Type::F32, Type::F64}, new TypeF64{}},
//     {{Type::F64, Type::F32}, new TypeF64{}},
//
//     {{Type::I32, Type::I64}, new TypeI64{}},
//     {{Type::I64, Type::I32}, new TypeI64{}},
// };


inline const std::unordered_map<std::string, uint8_t> keywords = {
    {"return", 1},  {"if", 2},      {"else", 3},
    {"for", 4},       {"while", 5},   {"is", 6},      {"in", 7},
    {"or", 8},        {"and", 9},     {"class", 10},  {"public", 11},
    {"private", 12},  {"const", 15},  {"static", 16}, {"break", 17},
    {"continue", 18}, {"elif", 19},   {"global", 20}, {"importc", 21},
    {"typedef", 22},  {"import", 23}, {"export", 24}, {"from", 25},
    {"var", 26},      {"fn", 27}, {"volatile", 28}, {"struct", 29}
};


// map, {operator, precedence}
inline std::unordered_map<std::string, int> operators = {
    {"=", 0},

    // Logical Operators
    {"||", 0},
    {"&&", 1},
    {"!=", 2},
    {"==", 2},

    // Relational Operators
    {">", 4},
    {"<", 4},
    {"<=", 4},
    {">=", 4},

    // Arithmetic Operators
    {"+", 8},
    {"-", 8},
    {"*", 16},
    {"/", 16},

    // Misc
    {".", 32}
};
