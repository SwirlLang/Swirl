#include <vector>
#include <variant>
#include <string_view>
#include <algorithm>
#include <iostream>
#include <tuple>

#ifndef CLI_H_Swirl
#define CLI_H_Swirl

const std::string USAGE = R"(The Swirl compiler
Usage: Swirl <input-file> [flags]

Flags:
)";

struct Argument {
	std::tuple<std::string, std::string> flags;
	std::string desc;

	bool value_required;
	std::string value;
};	

class cli {
private:
	int m_argc;
	const char ** m_argv;

	std::vector<Argument> m_flags, m_args;

private:
	std::vector<Argument> parse(int argc, const char **argv, const std::vector<Argument>& flags);

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
    std::variant<std::string, bool> get_flag(std::string_view flag);

public:
	cli(int argc, const char** argv, const std::vector<Argument>& flags);

public:
	bool contains_flag(std::string_view flag);
	std::string get_flag_value(std::string_view flag);
    std::string generate_help();

	std::string get_file();
};

#endif
