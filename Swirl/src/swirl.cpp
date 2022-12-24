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
#include <cli/argParser.h>

bool SW_DEBUG = false;
std::string SW_FED_FILE_PATH;
std::string SW_OUTPUT;
std::string SW_FED_FILE_SOURCE;

const char* SW_help = R"(The Swirl compiler

Usage: Swirl <file-path> [-o] <output> [--debug]

Flags:
    -d, --debug     log out steps of the compilation
    -o, --output    Output file name
    -r, --run       Run the compiled file
    -c, --compiler  C++ compiler to use
    -h, --help      Show this help message

Use swirl [command] --help for more information about the command)";

int main(int argc, char* argv[]) {
    parse(argc, argv);
}


// int main(int argc, const char* argv[]) {
//     std::string cxx = "g++";
//     defs defs{};
//     std::vector<std::string> args(argv, argv + argc);

//     if (argc <= 1) { std::cout << SW_help << std::endl;}
//     if (argc > 1) {
//         if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
//             std::cout << SW_help << std::endl;
//             return 0;
//         }

//         SW_FED_FILE_PATH = argv[1];
//         if (!std::filesystem::exists(SW_FED_FILE_PATH)) {
//             std::cerr << "File " << SW_FED_FILE_PATH << " doesn't exists!" << std::endl;
//             return 1;
//         }

//         std::ifstream fed_file_src_buf(SW_FED_FILE_PATH);
//         std::string src_current_ln;
//         while (std::getline(fed_file_src_buf, src_current_ln)) {
//             SW_FED_FILE_SOURCE += src_current_ln + "\n";
//         }

//         std::string cache_dir = getWorkingDirectory(SW_FED_FILE_PATH) + PATH_SEP + "__swirl_cache__" + PATH_SEP;
//         bool _debug;
//         if (std::find(args.begin(), args.end(), "-d") != args.end() ||
//             std::find(args.begin(), args.end(), "--debug") != args.end())
//             _debug = true;
        
//         if (std::find(args.begin(), args.end(), "-c") != args.end()) {
//             cxx = args[std::find(args.begin(), args.end(), "-c") - args.begin() + 1];
//         }

//         if (std::find(args.begin(), args.end(), "--compiler") != args.end()) {
//             cxx = args[std::find(args.begin(), args.end(), "--compiler") - args.begin() + 1];
//         }

//         preProcess(SW_FED_FILE_SOURCE, cache_dir);
//         InputStream is(SW_FED_FILE_SOURCE);
//         TokenStream tk(is, _debug);

//         Parser parser(tk);
//         parser.dispatch();
// //        for ( auto const& chl : parser.m_AST->chl) {
// //            std::cout << chl.type << std::endl;
// //        }

//         std::string file_name = SW_FED_FILE_PATH.substr(SW_FED_FILE_PATH.find_last_of("/\\") + 1);
//         std::string out_dir = SW_FED_FILE_PATH.replace(SW_FED_FILE_PATH.find(file_name),file_name.length(),"");
//         file_name = file_name.substr(0, file_name.find_last_of("."));

//         int o_loc = 0;
//         for (int i = 0; i < args.size(); ++i)
//             if (args[i] == "-o")
//                 o_loc = i + 1;

//         if (o_loc != 0)  SW_OUTPUT = args[o_loc];
//         if (SW_OUTPUT.empty()) SW_OUTPUT = file_name;
//         Transpile(*parser.m_AST, cache_dir + SW_OUTPUT + ".cpp");

//         if (std::find(args.begin(), args.end(), "--run") != args.end()) {
//             std::string cpp_obj =
//                     cxx + " " + cache_dir + SW_OUTPUT + ".cpp" + " -o " + out_dir + SW_OUTPUT + " && " + "." + PATH_SEP +
//                     out_dir + SW_OUTPUT;
//             system(cpp_obj.c_str());
//         }
//     }
// }

