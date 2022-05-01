/* Contain functions and classes to allow Swirl to interact with Operating system.

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

#ifndef Swirl_OS_H
#define Swirl_OS_H
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

using std::cout;
using std::endl;

namespace OS
{
    std::string platform();

    void mkdir(std::string &_dirPath);

    void mkdir(std::string &_dirPaths);

    void sys(std::string command);

    void rmdir(std::string &_dirPath);

    void rename(std::string &_oldName, std::string &_newName);

    void cpy(std::string &_from, std::string &_to);

    bool isDir(std::string &_path);

    bool isExists(std::string &_path);

    std::string sep();

    void rmdir(std::string path);
}
#endif
