#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "parsers/functions/FunctionParser.h"
#include "swirl.h"

bool swirl_DEBUG = false;
std::string swirl_FEEDED_FILE_PATH;
std::string swirl_OUTPUT;
std::string swirl_FEEDED_FILE_SOURCE;

int main(int argc, const char *argv[])
{
    if (argc <= 1)
    {
        std::ifstream lc_help_buf("res/cli_help.txt");
        std::string lc_help;

        std::cout << "\n";
        while (std::getline(lc_help_buf, lc_help))
            std::cout << lc_help << std::endl;
        std::cout << "\n";
    }

    if (argc > 1)
    {
        swirl_FEEDED_FILE_PATH = argv[1];
        std::ifstream feeded_file_src_buf(swirl_FEEDED_FILE_PATH);
        std::string src_current_ln;
        while (std::getline(feeded_file_src_buf, src_current_ln))
            swirl_FEEDED_FILE_SOURCE += src_current_ln + "\n";
    }
}
