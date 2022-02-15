#include <iostream>
#include <string>
#include <fstream>


const std::string FILE_PATH;

bool linting = false;
bool debug = false;


int main(int argc, std::string _filePath) {

    if (_filePath) {
        std::ifstream ascii_file("res/lc_ascii.txt");
        std::string line;
        std::string ascii_file_content;

        while (std::getline(ascii_file, line)) {
            ascii_file_content.append(line);
        }
    }

}