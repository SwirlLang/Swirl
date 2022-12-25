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
	//Use swirl [command] --help for more information about the command

	std::vector<Argument> parse(int argc, const char** argv, const std::vector<Argument>& flags);
    std::variant</* The flag value */ std::string, /* The flag is supplied or not */ bool>
        get_flag(std::string_view flag, const std::vector<Argument>& args);
}

#endif // !SWIRL_CLI_H
