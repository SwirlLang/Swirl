#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include <pre-processor/pre-processor.h>
#include <swirl.typedefs/swirl_t.h>
#include <tokenizer/tokenizer.h>
//#include <transpiler/transpiler.h>

bool swirl_DEBUG = false;
std::string swirl_FEEDED_FILE_PATH;
std::string swirl_OUTPUT;
std::string swirl_FEEDED_FILE_SOURCE;

char swirl_help[] = R"(The Swirl compiler

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

    if (argc <= 1) {
        std::cout << swirl_help << std::endl;
    }

    if (argc > 1) {
        swirl_FEEDED_FILE_PATH = argv[1];
        std::ifstream feeded_file_src_buf(swirl_FEEDED_FILE_PATH);
        std::string src_current_ln;
        while (std::getline(feeded_file_src_buf, src_current_ln)){
            swirl_FEEDED_FILE_SOURCE += src_current_ln + "\n";
        }
        preProcess(swirl_FEEDED_FILE_SOURCE, swirl_FEEDED_FILE_PATH, pre_processor_exit_code);
        tokenize(swirl_FEEDED_FILE_SOURCE, swirl_FEEDED_FILE_PATH);
        
        std::string file_name = swirl_FEEDED_FILE_PATH.substr(swirl_FEEDED_FILE_PATH.find_last_of("/\\") + 1);
        std::string out_dir = swirl_FEEDED_FILE_PATH.replace(swirl_FEEDED_FILE_PATH.find(file_name),file_name.length(),"");
        file_name = file_name.substr(0, file_name.find_last_of("."));
        
        std::string cpp_obj = "g++ " + getWorkingDirectory(swirl_FEEDED_FILE_PATH) + PATH_SEP + "__swirl_cache__" + PATH_SEP + file_name +".cpp" + " -o " + out_dir + file_name + " && " + "./" + out_dir + file_name;
        system(cpp_obj.c_str());
    }
}

