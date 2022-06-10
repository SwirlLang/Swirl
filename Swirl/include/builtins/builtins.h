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

// #include "utils/utils.h"
// #include "swirl.typedefs/swirl_t.h"

#ifndef BUILTINS_H_Swirl
#define BUILTINS_H_Swirl

std::variant<F_IO_Object::R_ModeObject,
             F_IO_Object::W_ModeObject,
             F_IO_Object::DualModeObject>
open(
    Swirl::string _filePath,
    Swirl::string _mode = "r",
    Swirl::string _encoding = "utf8");
template <typename T>
void print(T string, const char *end = "\n")
{
}

std::string input(const char *prompt)
{
}

template <typename T>
int len(T iterable)
{
}

auto range(int start, int end = 0)
{
}

#endif
