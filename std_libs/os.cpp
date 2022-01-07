/* Contains functions and classes to to allow LC to interact with Operating system */

#include <iostream>
#include <string>
#include <filesystem>
#include <direct.h>

namespace OS
{
    std::string& platform() {
        /*
        Returns a string containing the name of the OS the program
        is currently running on
        */

        std::string platform;

        #ifdef _WIN32
            platform = "WIN";

        #elif __linux__
            platform = "linux";

        #elif TARGET_OS_MAC
            platform = "darwin";

        #elif __ANDROID__
            platform = "android";

        #elif __unix__
            platform = "unix";
        #endif

        return platform;
    }

    void mkdir(std::string* _path) {
        /* Creates a new directory at the specified path */
        _mkdir((const char*)_path);
    }

}
