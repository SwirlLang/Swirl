#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "parsers/functions/FunctionParser.h"
#include "lambda-code.h"

bool LCC_DEBUG = false;
std::string LCC_FEEDED_FILE_PATH;
std::string LCC_OUTPUT;
std::string LCC_FEEDED_FILE_SOURCE;

int main(int argc, const char* argv[]) {
    if (argc <= 1) {
        std::ifstream lc_help_buf("res/cli_help.txt");
        std::string lc_help;

        std::cout << "\n";
        while (std::getline(lc_help_buf, lc_help)) 
            std::cout << lc_help << std::endl;
        std::cout << "\n";
    }

    if (argc > 1) {
        LCC_FEEDED_FILE_PATH = argv[1];
        std::ifstream feeded_file_src_buf(LCC_FEEDED_FILE_PATH);
        std::string src_current_ln;
        while (std::getline(feeded_file_src_buf, src_current_ln))
            LCC_FEEDED_FILE_SOURCE += src_current_ln + "\n";
        
    }
}
