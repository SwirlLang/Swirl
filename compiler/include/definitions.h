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


#define SW_BUILTIN_FILE_PATH "/__Sw__9117778/builtins.sw"

#define SW_BUILTIN_SOURCE ""
// #define SW_BUILTIN_SOURCE std::format(R"(
// export struct str {{
//     var __Sw_buffer: *char;
//     var __Sw_length: i64;
//
//     fn size(&self):  i64  {{ return self.__Sw_length; }}
//     fn ptr (&self): *char {{ return self.__Sw_buffer; }}
// }}
//
//
// export enum target {{
//     windows,
//     linux,
//     darwin,
//
//     x86,
//     x64,
//     arm64,
//     unknown
// }}
//
//
// export comptime let arch:     target = target::{};
// export comptime let platform: target = target::{};
//
// )",                                                                      \
// LLVMTargetTriple.getOS() == Triple::Win32 ? "windows"                        \
// : LLVMTargetTriple.getOS() == Triple::Linux ? "linux"                    \
// : LLVMTargetTriple.getOS() == Triple::MacOSX ? "darwin" : "unknown",     \
// \
// LLVMTargetTriple.getArch() == Triple::x86 ? "x86"                            \
// : LLVMTargetTriple.getArch() == Triple::x86_64 ? "x64"                   \
// : LLVMTargetTriple.getArch() == Triple::aarch64 ? "arm64" : "unknown")
