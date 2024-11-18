#include <fstream>


#if defined(_WIN32)
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif


std::string getWorkingDirectory(const std::string& _path) {
    if (_path.find_last_of(PATH_SEP) == std::string::npos)
        return ".";
    return _path.substr(0, _path.find_last_of(PATH_SEP));
}


