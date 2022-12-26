#ifndef SWIRL_CLI_H
#define SWIRL_CLI_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <tuple>
#include <sstream>
#include <variant>

namespace cli {
    struct Argument {
        std::tuple<std::string, std::string> flags;
        std::string desc;

        bool value_required;
        std::string value;
    };	

	const std::string USAGE = R"(The Swirl compiler
Usage: Swirl <input-file> [flags]

Flags:
)";

    std::string generate_help(const std::vector<Argument> &flags);

	std::vector<Argument> parse(int argc, const char** argv, const std::vector<Argument>& flags);
    std::variant</* The flag value */ std::string, /* The flag is supplied or not */ bool>
        get_flag(std::string_view flag, const std::vector<Argument>& args);
}

#endif // !SWIRL_CLI_H
