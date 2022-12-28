#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
 
#include <cli/cli.h>
#include <pre-processor/pre-processor.h>
#include <swirl.typedefs/swirl_t.h>
#include <tokenizer/InputStream.h>
#include <tokenizer/Tokenizer.h>
#include <definitions/definitions.h>
#include <transpiler/transpiler.h>
#include <parser/parser.h>

bool SW_DEBUG = false;
std::string SW_FED_FILE_PATH;
std::string SW_OUTPUT;
std::string SW_FED_FILE_SOURCE;

const std::vector<cli::Argument> application_flags = {
    {{"-h","--help"}, "Shows this help message", false, {}},
    {{"-r", "--run"}, "Run the compiled file", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-c", "--compiler"}, "C++ compiler to use", true, {}},
    {{"-d", "--debug"}, "Log the steps of compilation", false, {}}
};

int main(int argc, const char** argv) {
    std::vector<cli::Argument> args = cli::parse(argc, argv, application_flags);

    if (std::get<bool>(cli::get_flag("-h", args))) { 
        std::cout << cli::generate_help(application_flags) << '\n';
        return 0;
    }

    std::string cxx = "g++";
    auto comp_flg = cli::get_flag("-c", args);
    if (std::get_if<bool>(&comp_flg) == nullptr) /* The flag is supplied */
        cxx = std::get<std::string>(comp_flg);

	for (int i = 1; i < argc; i++) {
		if (
			argv[i][0] != '-' &&
			(argv[i - 1][0] != '-' ||
			std::find_if(application_flags.begin(), application_flags.end(), [&](const cli::Argument& _arg) {
				if (_arg.value_required) return false;
				auto &[v1, v2] = _arg.flags;
				return v1 == argv[i - 1] || v2 == argv[i - 1];
			}) != application_flags.end()
		)) {
			SW_FED_FILE_PATH = argv[i];
			break;
		}
	}

	if (!std::filesystem::exists(SW_FED_FILE_PATH)) {
		std::cerr << "File " << SW_FED_FILE_PATH << " doesn't exists!" << std::endl;
		return 1;
	}

	std::ifstream fed_file_src_buf(SW_FED_FILE_PATH);
    SW_FED_FILE_SOURCE = {
        std::istreambuf_iterator<char>(fed_file_src_buf),
        {}
    };
    fed_file_src_buf.close();

	std::string cache_dir = getWorkingDirectory(SW_FED_FILE_PATH) + PATH_SEP + "__swirl_cache__" + PATH_SEP;
    bool _debug = std::get<bool>(cli::get_flag("-d", args));

	preProcess(SW_FED_FILE_SOURCE, cache_dir);
	InputStream is(SW_FED_FILE_SOURCE);
	TokenStream tk(is, _debug);

	Parser parser(tk);
	parser.dispatch();

	std::string file_name = SW_FED_FILE_PATH.substr(SW_FED_FILE_PATH.find_last_of("/\\") + 1);
	std::string out_dir = SW_FED_FILE_PATH.replace(SW_FED_FILE_PATH.find(file_name),file_name.length(),"");
	file_name = file_name.substr(0, file_name.find_last_of("."));

	SW_OUTPUT = file_name;

    auto output_flg = cli::get_flag("-o", args);
    if (std::get_if<bool>(&output_flg) == nullptr)
        SW_OUTPUT = std::get<std::string>(output_flg);

	Transpile(*parser.m_AST, cache_dir + SW_OUTPUT + ".cpp");

	if (std::get<bool>(cli::get_flag("-r", args))) {
		std::string cpp_obj =
				cxx + " " + cache_dir + SW_OUTPUT + ".cpp" + " -o " + out_dir + SW_OUTPUT + " && " + "." + PATH_SEP +
				out_dir + SW_OUTPUT;
		system(cpp_obj.c_str());
	}
}
