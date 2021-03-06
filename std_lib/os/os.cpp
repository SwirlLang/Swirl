/*
Copyright (C) 2022 Swirl Organization

This file is part of the Swirl programming language

Swirl is free software: you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Swirl is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see https://www.gnu.org/licenses/.
*/

#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <string.h>

#include "swirl.typedefs/swirl_t.h"

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#ifdef _WIN32
#define platform_ "win"
#elif __linux__
#define platform_ "linux"
#elif TARGET_OS_MAC
#define platform_ "darwin"
#elif __ANDROID__
#define platform_ "android"
#elif __unix__
#define platform_ "unix"
#endif

using std::cout;
using std::endl;

namespace OS
{
    namespace fs = std::filesystem;
    Swirl::string platform()
    {
        return Swirl::string(platform_);
    }

    void mkdir(Swirl::string _dirPath)
    {
        fs::create_directory(_dirPath.__to_cstr__());
    }

    void mkdirs(Swirl::string _dirPaths)
    {
        fs::create_directories(_dirPaths.__to_cstr__());
    }

    void sys(Swirl::string command)
    {
        system(command.__to_cstr__());
    }

    void rmdir(Swirl::string _dirPath)
    {
        fs::remove(_dirPath.__to_cstr__());
    }

    void rename(Swirl::string _oldName, Swirl::string _newName)
    {
        fs::rename(_oldName.__to_cstr__(), _newName.__to_cstr__());
    }

    void cpy(Swirl::string _from, Swirl::string _to)
    {
        fs::copy(_from.__to_cstr__(), _to.__to_cstr__());
    }

    bool isDir(Swirl::string _path)
    {
        return fs::is_directory(_path.__to_cstr__());
    }

    bool isExists(Swirl::string _path)
    {
        return fs::exists(_path.__to_cstr__());
    }

    std::string sep()
    {
        return PATH_SEPARATOR;
    }
}
