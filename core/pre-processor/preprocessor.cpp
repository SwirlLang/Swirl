#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <any>

#include "../utils/utils.h"


// void preProcess(std::string _filePath, bool& exitCode) {
//     std::ifstream r_file(_filePath);
//     std::string line;

//     std::vector<std::string> parsed_imports = {};

//     while (std::getline(r_file, line)) {
//         if (line.find("#") != std::string::npos) {
//             strtok(line.c_str(), "#");
//         }
//     }

// }

int main()
{
    auto e = strtok("#include <iostream>\n#include <string>\n\n\nclass String {", "#");

    std::cout << e << std::endl;
    return 0;
}