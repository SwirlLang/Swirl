#include "cli/cli.h"

namespace cli {
	std::string generate_help(const std::vector<Argument>& flags) {
		int max_width = 0;
		std::for_each(flags.cbegin(), flags.cend(), [&](const Argument& arg) {
			auto& [v1, v2] = arg.flags;
			if (v1.size() + v2.size() + 2 > max_width) max_width = v1.size() + v2.size() + 3;
		});

		std::string msg;
		for (const Argument& arg : flags) {
			auto& [v1, v2] = arg.flags;
			msg += "\t" + v1 + ", " + v2;
			for (int c = 0; c < max_width - v1.size() - v2.size() - 2; c++) msg += ' ';
			msg += "     " + arg.desc + '\n';
		} return msg;
	}

	std::vector<Argument> parse(int argc, const char** argv, const std::vector<Argument>& flags) {
		if (argc <= 1) { 
			std::cout << USAGE << generate_help(flags) << '\n';
			exit(0); 
		}

		std::vector<std::string_view> args(argv, argv + argc);
		std::vector<Argument> supplied;

		for (auto arg_iterator = args.cbegin() + 1; arg_iterator != args.cend(); ++arg_iterator) {
			// if the current argument starts with `-` sign, its a flag
			if (arg_iterator->starts_with("-")) {

				// check if the flag exists in the flag vector
				auto it = std::find_if(flags.cbegin(), flags.cend(), [&](const Argument& a) {
					auto& [v1, v2] = a.flags;
					return v1 == *arg_iterator || v2 == *arg_iterator;
					});

				if (it == flags.cend()) { std::cerr << "Unknown flag: " << *arg_iterator << '\n'; exit(1); }

				if (!it->value_required) supplied.push_back(*it);
				else {
					if (arg_iterator + 1 == args.cend()) { std::cout << "Value missing for the flag: " << *arg_iterator << '\n'; exit(1); }

					Argument _arg = *it;
					_arg.value = *(arg_iterator + 1);
					supplied.push_back(_arg);
				}

			}
		}

		return supplied;
	}

    std::variant</* The flag value */ std::string, /* The flag is supplied or not */ bool>
		get_flag(std::string_view flag, const std::vector<Argument>& args) {
		auto it = std::find_if(args.cbegin(), args.cend(), [&](const Argument& a) {
			auto& [v1, v2] = a.flags;
			return v1 == flag || v2 == flag;
		});

		// Flag not found
		if (it == args.cend()) return false;
		
		// Flag exists
		if (!it->value_required) return true;

		return it->value;
	}
}