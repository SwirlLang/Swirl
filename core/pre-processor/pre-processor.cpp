#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#include "pre-processor.h"
#include "../utils/utils.h"

template <typename Any>
void preProcess(std::string& _filePath, Any _stringIndices, uint8_t& preProcessorExitCode) {
    /*
     * For now the pre processor handles injection of foreign namespaces into another namespace.
     * A general structure of String indices should be a 2D array of range functions,
     * like so, {{{}, {}}, {{}, {}}]
     * 'range' function is defined in utils.h 
     */

    
    std::ifstream pp_file_buf(_filePath);
    std::string current_ln;

    while (std::getline(pp_file_buf, current_ln)) {
        if (current_ln._Starts_with("use")) {
            std::string i_module = current_ln.erase(0, current_ln.find("use") + 4).replace(".", "/");
            try {
                std::ofstream local_module_buf(i_module);
                local_module_buf.write()
                
            } 
            catch(const std::exception& e) {
                
            }  
        }
    }

    preProcessorExitCode = 0;
}