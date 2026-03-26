#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
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
    "true", "false", "undefined", "enum", "protocol",
    "const", "static", "break", "continue", "elif",
    "extern", "comptime", "let", "import", "export",
    "var", "fn", "volatile", "struct"
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

#define SW_BUILTIN_FILE_PATH "/__Sw__9117778/builtins.sw"

#define SW_BUILTIN_SOURCE std::format(R"(
export struct str {{
    var __Sw_buffer: *char;
    var __Sw_length: i64;

    fn size(&self):  i64  {{ return self.__Sw_length; }}
    fn ptr (&self): *char {{ return self.__Sw_buffer; }}
}}

// export comptime platform = "{}";
// export comptime arch     = "{}";
)",                                                                      \
LLVMTargetTriple.getOS() == Triple::Win32 ? "windows"                        \
: LLVMTargetTriple.getOS() == Triple::Linux ? "linux"                    \
: LLVMTargetTriple.getOS() == Triple::MacOSX ? "darwin" : "unknown",     \
\
LLVMTargetTriple.getArch() == Triple::x86 ? "x86"                            \
: LLVMTargetTriple.getArch() == Triple::x86_64 ? "x64"                   \
: LLVMTargetTriple.getArch() == Triple::aarch64 ? "arm64" : "unknown")
