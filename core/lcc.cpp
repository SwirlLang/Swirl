#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "parsers/functions/FunctionParser.h"

bool LCC_DEBUG = false;
std::string LCC_FEEDED_FILE;
std::string LCC_OUTPUT;

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
        LCC_FEEDED_FILE = argv[1];
    }
}
