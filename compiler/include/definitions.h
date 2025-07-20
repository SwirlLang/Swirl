#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <string_view>


struct Node;
using AST_t = std::vector<std::unique_ptr<Node>>;


consteval std::filesystem::path getSpmPkgInstallDir() {
#if defined(_WIN32) || defined(_WIN64)
    return "%APPDATA%\\local\\spm\\packages";
#elif defined(__linux__) || defined (__APPLE__)
    return "~/.spm/packages";
#else 
    #error "getSpmPkgInstallDir: unable to determine current platform!";
#endif
}


inline const std::unordered_set<std::string_view> keywords = {
    "return", "if", "else", "for", "while", "in",
    "or", "and", "class", "public", "private",
    "const", "static", "break", "continue", "elif",
    "extern", "importc", "let", "import", "export",
    "from", "var", "fn", "volatile", "struct"
};


// map, {operator, precedence}
inline std::unordered_map<std::string, int> operators = {
    {"=", 0},
    {"+=", 0},
    {"-=", 0},
    {"*=", 0},
    {"/=", 0},

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

    {"as", 19},
    {"!", 20},

    // Misc
    {"[]", 32},
    {".", 34},
};
