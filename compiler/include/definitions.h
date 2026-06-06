#pragma once
#include <vector>
#include <string_view>


struct Node;
using AST_t = std::vector<Node*>;


consteval std::string_view getBonoPackagesDir() {
#if defined(_WIN32) || defined(_WIN64)
    return "%APPDATA%\\local\\spm\\packages";
#elif defined(__linux__) || defined (__APPLE__)
    return "~/.spm/packages";
#else 
    #error "getBonoPackagesDir: unable to determine current platform!";
#endif
}