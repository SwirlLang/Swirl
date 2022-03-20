#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#include "pre-processor.h"
#include "../utils/utils.h"

template <typename Any>
void preProcess(std::string _filePath, Any _stringIndices, bool& preProcessorExitCode) {
    std::ifstream pp_file_buf(_filePath);
    std::string current_ln;

    while (std::getline(pp_file_buf, current_ln)) {
        if (current_ln.find("use") != std::string::npos) {
            std::string i_module = current_ln.erase(0, current_ln.find("use") + 4).replace(".", "/");
            try {
                std::ifstream local_module_fbuf(i_module);
                std::string local_module_current_ln;
                while (std::getline(local_module_fbuf, local_module_current_ln)) {

                }
            } catch(const std::exception& e) {
                
            }  
        }
    }
}