#pragma once
#include <string_view>

#ifdef __has_include(<print>)
#include <print>
#else
#include <iostream>
#include <format>
#include <ostream>
#endif


extern bool SW_IS_DEBUG;

namespace detail {
constexpr std::string_view ANSI_CODE_RED    = "\x1b[31m";
constexpr std::string_view ANSI_CODE_GREEN  = "\x1b[32m";
constexpr std::string_view ANSI_CODE_YELLOW = "\x1b[33m";
constexpr std::string_view ANSI_CODE_RESET  = "\x1b[0m";
constexpr std::string_view ANSI_CODE_CYAN   = "\033[36m";

enum class Color {
    CYAN,  YELLOW,
    GREEN, RED
};


inline std::string colorize(const std::string_view str, const Color color) {
    switch (color) {
        case Color::RED:    return std::format("{}{}{}", ANSI_CODE_RED,    str, ANSI_CODE_RESET);
        case Color::GREEN:  return std::format("{}{}{}", ANSI_CODE_GREEN,  str, ANSI_CODE_RESET);
        case Color::YELLOW: return std::format("{}{}{}", ANSI_CODE_YELLOW, str, ANSI_CODE_RESET);
        case Color::CYAN:   return std::format("{}{}{}", ANSI_CODE_CYAN,   str, ANSI_CODE_RESET);
        default:            throw std::runtime_error("Invalid color");
    }
}


template <typename... Args>
void formatted_log_write(std::format_string<Args...> fmt, Args... args) {
#ifdef __cpp_lib_print
    std::print(fmt, std::forward<Args>(args)...);
#else
    std::cout << std::format(fmt, std::forward<Args>(args)...) << std::flush;
#endif
}


template <typename... Args>
void formatted_log_write_line(std::format_string<Args...> fmt, Args... args) {
#ifdef __cpp_lib_print
    std::println(fmt, std::forward<Args>(args)...);
#else
    std::cout << std::format(fmt, std::forward<Args>(args)...) << std::endl;
#endif
}
}


#define SW_LOG_INFO(...) \
    do { \
        if (SW_IS_DEBUG) { \
            detail::formatted_log_write_line("{}{}", \
                detail::colorize("[INFO]  ", detail::Color::CYAN), \
                std::format(__VA_ARGS__)); \
        } \
    } while (0)


#define SW_LOG_WARN(...) \
    do { \
        if (SW_IS_DEBUG) { \
            detail::formatted_log_write_line("{}{}", \
                detail::colorize("[WARN]  ", detail::Color::YELLOW), \
                std::format(__VA_ARGS__)); \
        } \
    } while (0)
