/*
Copyright (C) 2022 Swirl Organization

This file is part of the Swirl programming language

Swirl is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation,
 either version 3 of the License, or (at your option) any later version.

Swirl is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/
*/

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <variant>

#include <utils/utils.h>
#include <swirl.typedefs/swirl_t.h>

// commented because the code has some errors
// std::variant<F_IO_Object::R_ModeObject,
//              F_IO_Object::W_ModeObject,
//              F_IO_Object::DualModeObject>
// open(
//     Swirl::string _filePath,
//     Swirl::string _mode = "r",
//     Swirl::string _encoding = "utf8")
// {
//     if (_mode.has("w"))
//         return F_IO_Object::W_ModeObject(_filePath);
//     else if (_mode.value == "r")
//         return F_IO_Object::R_ModeObject(_filePath);
//     else if (_mode.value == "w+" || _mode.value == "r+")
//         return F_IO_Object::DualModeObject(_filePath);
// }

template <typename T>
void print(T string, const char *end = "\n")
{
    /* writes to the standard output */
    std::cout << std::to_string(string) << end;
}

std::string input(const char *prompt)
{
    /* standard input */
    std::string ret;
    std::cout << prompt;
    std::getline(std::cin, ret);
    return ret;
}

template <typename T>
int len(T iterable)
{
    /* returns the length of an iterable */
    int ret = 0;
    for (auto item : iterable)
        ret++;

    return ret;
}

auto range(int start, int end = 0)
{
    /* returns an iterable vector containing a series of numbers as specified */

    std::vector<int> ret;

    if (!end)
        end = start;

    for (int i = start; i < end; i++)
    {
        ret.push_back(i);
    }
    return ret;
}