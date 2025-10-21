#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <string_view>


struct Node;
using AST_t = std::vector<std::unique_ptr<Node>>;


consteval std::string_view getBonoPackagesDir() {
#if defined(_WIN32) || defined(_WIN64)
    return "%APPDATA%\\local\\spm\\packages";
#elif defined(__linux__) || defined (__APPLE__)
    return "~/.spm/packages";
#else 
    #error "getBonoPackagesDir: unable to determine current platform!";
#endif
}


consteval std::string_view getSwInternalComponentDir() {
#if defined(_WIN32) || defined(_WIN64)
    return "C:\\Program Files\\swirl\\bin\\";
#elif defined(__linux__) || defined(__APPLE__)
    return "/usr/local/libexec/swirl/";
#else
    #error "getSwLLDPath: failed to query platform"
#endif
}


inline const std::unordered_set<std::string_view> KeywordSet = {
    "return", "if", "else", "for", "while", "mut",
    "true", "false", "class", "public", "protocol",
    "const", "static", "break", "continue", "elif",
    "extern", "comptime", "let", "import", "export",
    "var", "fn", "volatile", "struct",
};


inline std::unordered_set<std::string> OperatorSet = {
    "=", "+=", "-=", "*=", "/=", "%=",
    "::",
    "||", "&&", "!=", "==",
    ">", "<", "<=", ">=",
    "+", "-", "*", "/", "%", "//",
    "as", "!",
    "[]", "."
};
