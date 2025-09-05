#include "cli/cli.h"
#include <algorithm>

cli::cli(int argc, const char** argv, const std::vector<Argument>& flags):
    m_argc(argc),
    m_argv(argv),
    m_flags(&flags),
    m_input_file(),
    m_args(parse()) { }

bool cli::contains_flag(std::string_view flag) {
    return std::any_of(m_args.cbegin(), m_args.cend(), [&](const Argument& a) {
        auto& [f1, f2] = a.flags;
        return f1 == flag || f2 == flag;
    });
}

std::string cli::get_flag_value(std::string_view flag) {
    auto it = std::find_if(m_args.cbegin(), m_args.cend(), [&](const Argument& a) {
        auto& [f1, f2] = a.flags;
        return f1 == flag || f2 == flag;
    });
    if (it != m_args.cend() && !it->values.empty()) {
        return it->values.front();
    }
    return "";
}

std::vector<std::string> cli::get_flag_values(std::string_view flag) {
    auto it = std::find_if(m_args.cbegin(), m_args.cend(), [&](const Argument& a) {
        auto& [f1, f2] = a.flags;
        return f1 == flag || f2 == flag;
    });

    if (it != m_args.cend()) {
        return it->values;
    }
    return {};
}

std::string cli::generate_help() {
    std::size_t max_width = 0;
    std::for_each(m_flags->cbegin(), m_flags->cend(), [&](const Argument& arg) {
        auto& [f1, f2] = arg.flags;
        if (f1.size() + f2.size() + 2 > max_width) max_width = f1.size() + f2.size() + 3;
    });

    std::string msg;
    for (const Argument& arg : *m_flags) {
        auto& [f1, f2] = arg.flags;
        msg += "\t" + f1 + ", " + f2;
        for (unsigned int c = 0; c < max_width - f1.size() - f2.size() - 2; c++) msg += ' ';
        msg += "     " + arg.desc + '\n';
    } return msg;
}

std::optional<std::string> cli::get_file() {
    return m_input_file;
}

std::vector<Argument> cli::parse() {
    if (m_argc <= 1) { 
        std::cout << USAGE << generate_help() << '\n';
        exit(0); 
    }

    std::vector<std::string_view> args(m_argv, m_argv + m_argc);
    std::vector<Argument> supplied;

    for (auto arg_iterator = args.cbegin() + 1; arg_iterator != args.cend(); ++arg_iterator) {
        // if the current argument starts with `-` sign, its a flag
        if (arg_iterator->starts_with("-")) {

            // check if the flag exists in the flag vector
            auto flag_it = std::find_if(m_flags->cbegin(), m_flags->cend(), [&](const Argument& a) {
                auto& [f1, f2] = a.flags;
                return f1 == *arg_iterator || f2 == *arg_iterator;
            });

            if (flag_it == m_flags->cend()) { std::cerr << "Unknown flag: " << *arg_iterator << '\n'; exit(1); }

            // Check if this flag was already supplied
            auto supplied_it = std::find_if(supplied.begin(), supplied.end(), [&](const Argument& a) {
                return std::get<0>(a.flags) == std::get<0>(flag_it->flags);
            });

            if (supplied_it != supplied.end()) { // Flag was already seen
                if (!supplied_it->repeatable) {
                    std::cerr << "Flag " << *arg_iterator << " cannot be repeated.\n";
                    exit(1);
                }
                // It's repeatable, add the new value
                if (flag_it->value_required) {
                    if (arg_iterator + 1 == args.cend() || (*(arg_iterator + 1)).starts_with("-")) {
                        std::cout << "Value missing for the flag: " << *arg_iterator << '\n';
                        exit(1);
                    }
                    supplied_it->values.push_back(std::string(*(arg_iterator + 1)));
                    ++arg_iterator; // Consume the value
                }
            } else { // First time seeing this flag
                Argument _arg = *flag_it;
                if (_arg.value_required) {
                    if (arg_iterator + 1 == args.cend() || (*(arg_iterator + 1)).starts_with("-")) {
                        std::cout << "Value missing for the flag: " << *arg_iterator << '\n';
                        exit(1);
                    }
                    _arg.values.push_back(std::string(*(arg_iterator + 1)));
                    ++arg_iterator; // Consume the value
                }
                supplied.push_back(_arg);
            }
        }
        else {
            if(!m_input_file.empty()) {
                std::cerr << "Multiple input files provided. Only one input file is allowed.\n";
                exit(1);
            }
            m_input_file = *arg_iterator;
        }
    }

    return supplied;
}