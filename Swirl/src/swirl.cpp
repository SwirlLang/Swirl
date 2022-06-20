#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include <pre-processor/pre-processor.h>
#include <parsers/functions/FunctionParser.h>
#include <swirl.typedefs/swirl_t.h>

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
        while (std::getline(feeded_file_src_buf, src_current_ln))
            swirl_FEEDED_FILE_SOURCE += src_current_ln + "\n";
    }

    preProcess(swirl_FEEDED_FILE_SOURCE, swirl_FEEDED_FILE_PATH, pre_processor_exit_code);
}
