#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
 
#include <pre-processor/pre-processor.h>
#include <swirl.typedefs/swirl_t.h>
#include <tokenizer/InputStream.h>
#include <tokenizer/Tokenizer.h>
#include <definitions/definitions.h>
#include <transpiler/transpiler.h>
#include <parser/parser.h>

bool swirl_DEBUG = false;
std::string swirl_FED_FILE_PATH;
std::string swirl_OUTPUT;
std::string swirl_FED_FILE_SOURCE;

const char* swirl_help = R"(The Swirl compiler

Usage: swirl <file-path> [-o] <output> [--debug]

Commands:
    run     compile and run the executable

Flags:
    -d, --debug     log out steps of the compilation
    -o, --output    Output file name
    -h, --help      Show this help message

Use swirl [command] --help for more information about the command)";


int main(int argc, const char* argv[]) {
    defs defs{};
    std::vector<std::string> args(argv, argv + argc);
    if (argc <= 1) { std::cout << swirl_help << std::endl;}
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            std::cout << swirl_help << std::endl;
            return 0;
        }

        swirl_FED_FILE_PATH = argv[1];
        if (!std::filesystem::exists(swirl_FED_FILE_PATH)) {
            std::cerr << "File " << swirl_FED_FILE_PATH << " doesn't exists!" << std::endl;
            return 1;
        }
        std::ifstream fed_file_src_buf(swirl_FED_FILE_PATH);
        std::string src_current_ln;
        while (std::getline(fed_file_src_buf, src_current_ln)){
            swirl_FED_FILE_SOURCE += src_current_ln + "\n";
        }

        std::string cache_dir = getWorkingDirectory(swirl_FED_FILE_PATH) + PATH_SEP + "__swirl_cache__" + PATH_SEP;
        bool _debug;
        if (std::find(args.begin(), args.end(), "-d") != args.end() ||
            std::find(args.begin(), args.end(), "--debug") != args.end())
            _debug = true;

        preProcess(swirl_FED_FILE_SOURCE, cache_dir);
        InputStream is(swirl_FED_FILE_SOURCE);
        TokenStream tk(is, _debug);

        Parser parser(tk);
        parser.dispatch();
//        for ( auto const& chl : parser.m_AST->chl) {
//            std::cout << chl.type << std::endl;
//        }

        std::string file_name = swirl_FED_FILE_PATH.substr(swirl_FED_FILE_PATH.find_last_of("/\\") + 1);
        std::string out_dir = swirl_FED_FILE_PATH.replace(swirl_FED_FILE_PATH.find(file_name),file_name.length(),"");
        file_name = file_name.substr(0, file_name.find_last_of("."));

//        if (swirl_OUTPUT.empty()) swirl_OUTPUT = file_name;
        Transpile(*parser.m_AST, cache_dir + file_name + ".cpp");

        if (std::find(args.begin(), args.end(), "--run") != args.end()) {
            std::cout << "found";
            std::string cpp_obj =
                    "g++ " + cache_dir + file_name + ".cpp" + " -o " + out_dir + file_name + " && " + "." + PATH_SEP +
                    out_dir + file_name;

            system(cpp_obj.c_str());
        }
    }
}

