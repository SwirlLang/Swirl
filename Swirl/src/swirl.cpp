#include <iostream>
#include <string>
#include <fstream>
#include <vector>
 
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


int main(int argc, const char *argv[]) {
    uint8_t pre_processor_exit_code;
    defs defs{};

    if (argc <= 1)
        std::cout << swirl_help << std::endl;

    if (argc > 1) {
        swirl_FED_FILE_PATH = argv[1];
        std::ifstream fed_file_src_buf(swirl_FED_FILE_PATH);
        std::string src_current_ln;
        while (std::getline(fed_file_src_buf, src_current_ln)){
            swirl_FED_FILE_SOURCE += src_current_ln + "\n";
        }

        uint8_t _debug = strcmp(argv[0], "--debug") != 0;
        preProcess(swirl_FED_FILE_SOURCE, swirl_FED_FILE_PATH, pre_processor_exit_code);
        InputStream is(swirl_FED_FILE_SOURCE);
        TokenStream tk(is);
//        while (!tk.eof()) {
//            auto d_tok = tk.next();
//            std::cout << "TYPE: \t\t" << d_tok[0] << "\t VAL: \t\t" << d_tok[1] << std::endl;
//        }
        Parser parser(tk);
//        parser.dispatch();
        parser.dispatch();
//        for ( auto const& chl : parser.m_AST->chl) {
//            std::cout << chl.type << std::endl;
//        }
        Transpile(*parser.m_AST, swirl_OUTPUT);

        std::string file_name = swirl_FED_FILE_PATH.substr(swirl_FED_FILE_PATH.find_last_of("/\\") + 1);
        std::string out_dir = swirl_FED_FILE_PATH.replace(swirl_FED_FILE_PATH.find(file_name),file_name.length(),"");
        file_name = file_name.substr(0, file_name.find_last_of("."));
        
        std::string cpp_obj = "g++ " + getWorkingDirectory(swirl_FED_FILE_PATH) + PATH_SEP + "__swirl_cache__" + PATH_SEP + file_name +".cpp" + " -o " + out_dir + file_name + " && " + "." + PATH_SEP + out_dir + file_name;
        system(cpp_obj.c_str());
    }
}

