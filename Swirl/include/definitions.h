#pragma once
#include <string>
#include <cstdint>
#include <filesystem>
#include <unordered_map>


consteval std::filesystem::path getSpmPkgInstallDir() {
#if defined(_WIN32) || defined(_WIN64)
    return "%APPDATA%\\local\\spm\\packages";
#elif defined(__linux__) || defined (__APPLE__)
    return "~/.spm/packages";
#else 
    #error "getSpmPkgInstallDir: unable to determine current platform!";
#endif
}



inline const std::unordered_map<std::string, uint8_t> keywords = {
    {"return", 1},  {"if", 2},      {"else", 3},
    {"for", 4},       {"while", 5},   {"as", 6},      {"in", 7},
    {"or", 8},        {"and", 9},     {"class", 10},  {"public", 11},
    {"private", 12},  {"const", 15},  {"static", 16}, {"break", 17},
    {"continue", 18}, {"elif", 19},   {"global", 20}, {"importc", 21},
    {"typedef", 22},  {"import", 23}, {"export", 24}, {"from", 25},
    {"var", 26},      {"fn", 27}, {"volatile", 28}, {"struct", 29}
};


// map, {operator, precedence}
inline std::unordered_map<std::string, int> operators = {
    {"=", 0},
    {"::", 0},


    {"||", 0},
    {"&&", 1},
    {"!=", 2},
    {"==", 2},

    {">", 4},
    {"<", 4},
    {"<=", 4},
    {">=", 4},

    {"+", 8},
    {"-", 8},
    {"*", 16},
    {"/", 16},

    {"!", 20},

    // Misc
    {".", 32}
};
