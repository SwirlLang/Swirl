#ifndef SWIRL_CLI_H
#define SWIRL_CLI_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <tuple>
#include <variant>

namespace cli {
    struct Argument {
        std::tuple<std::string, std::string> flags;
        std::string desc;

        bool value_required;
        std::string value;
    };	

	
	constexpr std::string_view HELP_MESSAGE = R"(The Swirl compiler

Usage: Swirl <file-path> [-o] <output> [--debug]

Flags:
	-h, --help      Show this help message
	-d, --debug     log out steps of the compilation
	-o, --output    Output file name
	-r, --run       Run the compiled file
	-c, --compiler  C++ compiler to use
)";

	std::vector<Argument> parse(int argc, const char** argv, const std::vector<Argument>& flags);
    /**
     * This function returns the value of the flag requested from the provided args vector.
     * If the flag is not supplied, it returns false.
     * If the flag is supplied, it returns the value of the flag.
     * If the flag is not required to have a value, it returns true.
     *
     * @param flag Requested flag
     * @param args Vector of arguments
     * @return The value of the flag
     */
    std::variant<std::string, bool>
        get_flag(std::string_view flag, const std::vector<Argument>& args);
}

#endif // !SWIRL_CLI_H
