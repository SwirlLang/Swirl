#include <iostream>
#include <vector>
#include <fstream>

#ifndef UTILS_H_Swirl
#define UTILS_H_Swirl

struct F_IO_Object
{
    class R_ModeObject;

    class W_ModeObject;

    class DualModeObject;
};

std::string getWorkingDirectory(const std::string&);

bool isInString(std::size_t, std::string);

template <typename Indices>
bool isInsideString(std::string &source, std::string substr, Indices stringIndices);

std::string getPathSep();

std::vector<int> findAllOccurrences(std::string &str, char substr);

std::string splitString(std::string string, char delimeter);

std::vector<std::string> splitIntoIterable(std::string string, char delimeter);

#endif