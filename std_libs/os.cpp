/* Contains functions and classes to to allow LC to interact with Operating system 

Copyright (C) 2022 Lambda Code Organization

This file is part of the Lambda Code programming language

Lambda Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Lambda Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
*/

#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <string.h>

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

namespace OS
{
    namespace fs = std::filesystem;
    std::string platform()
    {
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

    void mkdir(std::string &_dirPath)
    {
        fs::create_directory(_dirPath);
    }

    void mkdir(std::string &_dirPaths)
    {
        fs::create_directories(_dirPaths);
    }

    void sys(std::string command)
    {
        system((const char *)&command);
    }

    void rmdir(std::string &_dirPath)
    {
        fs::remove(_dirPath);
    }

    void rename(std::string &_oldName, std::string &_newName)
    {
        fs::rename(_oldName, _newName);
    }

    void cpy(std::string &_from, std::string &_to)
    {
        fs::copy(_from, _to);
    }

    bool isDir(std::string &_path)
    {
        return fs::is_directory(_path);
    }

    bool isExists(std::string &_path)
    {
        return fs::exists(_path);
    }

    std::string sep()
    {
        return PATH_SEPARATOR;
    }
}
