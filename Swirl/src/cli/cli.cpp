#include "cli/cli.h"

namespace cli {
	std::vector<Argument> parse(int argc, const char** argv, const std::vector<Argument>& flags) {
		if (argc <= 1) { std::cout << HELP_MESSAGE; exit(0); }

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
    
    std::variant<std::string, bool>
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