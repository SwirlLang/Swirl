#pragma once
#include <array>
#include <string>
#include <vector>
#include <variant>
#include <string_view>

const std::string USAGE = R"(The Swirl compiler
Usage: Swirl <input-file> [flags]

Flags:
)";

struct Argument {
    std::array<std::string, 2> flags;
    std::string desc;

    bool value_required;
    bool repeatable = false;         // Add this
    std::vector<std::string> values; // Change this from std::string
};

class cli {
  public:
    cli(int argc, const char** argv, const std::vector<Argument>& flags);

    bool contains_flag(std::string_view flag);
    std::string get_flag_value(std::string_view flag);
    std::vector<std::string> get_flag_values(std::string_view flag); // Add this
    std::string generate_help();

    std::string get_file();

  private:
    std::vector<Argument> parse();

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

    int m_argc;
    const char** m_argv;

    const std::vector<Argument>* m_flags;
    std::string m_input_file;
    std::vector<Argument> m_args;
};