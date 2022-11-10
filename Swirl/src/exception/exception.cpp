#include <iostream>
#include <map>
#include <sstream>

extern std::string swirl_FED_FILE_SOURCE;

void raiseException(const char* _msg, std::map<const char*, std::size_t> _prsState) {
    std::stringstream src_stream(swirl_FED_FILE_SOURCE);
    std::size_t cl_index;
    for (std::string cur_ln; std::getline(src_stream, cur_ln); cl_index++) {
        if (cl_index == _prsState["LINE"]) {
            std::cout << cur_ln << std::endl;
        }
    }
}