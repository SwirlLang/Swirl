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

const std::vector<Argument> application_flags = {
    {{"-h","--help"}, "Shows this help message", false, {}},
    {{"-r", "--run"}, "Run the compiled file", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-c", "--compiler"}, "C++ compiler to use", true, {}},
    {{"-d", "--debug"}, "Log the steps of compilation", false, {}}
};

int main(int argc, const char ** const argv) {
	cli app(argc, argv, application_flags);

    if (app.contains_flag("-h")) {
        std::cout << USAGE << app.generate_help() << '\n';
        return 0;
    }

	std::string cxx;

	if (app.contains_flag("-c"))
		cxx = app.get_flag_value("-c");
	else cxx = "g++";

	SW_FED_FILE_PATH = app.get_file();

	if (!std::filesystem::exists(SW_FED_FILE_PATH)) {
		std::cerr << "File '" << SW_FED_FILE_PATH << "' doesn't exists!" << std::endl;
		return 1;
	}

	std::ifstream fed_file_src_buf(SW_FED_FILE_PATH);
    SW_FED_FILE_SOURCE = {
        std::istreambuf_iterator<char>(fed_file_src_buf),
        {}
    };
    fed_file_src_buf.close();

	std::string cache_dir = getWorkingDirectory(SW_FED_FILE_PATH) + PATH_SEP + "__swirl_cache__" + PATH_SEP;
    bool _debug = app.contains_flag("-d");

	preProcess(SW_FED_FILE_SOURCE, cache_dir);
	InputStream is(SW_FED_FILE_SOURCE);
	TokenStream tk(is, _debug);

	Parser parser(tk);
	parser.dispatch();

	std::string file_name = SW_FED_FILE_PATH.substr(SW_FED_FILE_PATH.find_last_of("/\\") + 1);
	std::string out_dir = SW_FED_FILE_PATH.replace(SW_FED_FILE_PATH.find(file_name),file_name.length(),"");
	file_name = file_name.substr(0, file_name.find_last_of("."));

	SW_OUTPUT = file_name;

	if (app.contains_flag("-o"))
		SW_OUTPUT = app.get_flag_value("-o");

	Transpile(*parser.m_AST, cache_dir + SW_OUTPUT + ".cpp");

	if (app.contains_flag("-r")) {
		std::string cpp_obj =
				cxx + " " + cache_dir + SW_OUTPUT + ".cpp" + " -o " + out_dir + SW_OUTPUT + " && " + "." + PATH_SEP +
				out_dir + SW_OUTPUT;
		system(cpp_obj.c_str());
	}
}
