#pragma once
#include <array>
#include <string>
#include <vector>
#include <string_view>


struct Argument {
    std::array<std::string, 2> flags;
    std::string desc;

    bool value_required{};
    bool repeatable = false; 
    std::vector<std::string> values;
};


class cli {
  public:
    cli(int argc, const char** argv, const std::vector<Argument>& flags);

    [[nodiscard]] std::string generate_help() const;
    [[nodiscard]] std::string get_flag_value(std::string_view flag) const;
    [[nodiscard]] std::vector<std::string> get_flag_values(std::string_view flag) const;

    [[nodiscard]] bool contains_flag(std::string_view flag) const;

    static
    std::string print_usage(const std::string& exe_name);
    std::string get_file();

  private:
    std::vector<Argument> parse();

    int m_argc;
    const char** m_argv;

    const std::vector<Argument>* m_flags;
    std::string m_input_file;
    std::vector<Argument> m_args;
};