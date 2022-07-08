#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#ifndef UTILS_H_Swirl
#define UTILS_H_Swirl

struct F_IO_Object
{
    class R_ModeObject;

    class W_ModeObject;

    class DualModeObject;
};

void replaceAll(std::string&, std::string, std::string);

std::string getWorkingDirectory(const std::string&);

bool isInString(std::size_t, std::string);

template <typename Indices>
bool isInsideString(std::string &source, std::string substr, Indices stringIndices);

#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

//template <class Type, typename T>
//std::size_t getIndex(std::vector<Type>, T);

//template <class Type>
std::vector<std::string> sliceVec(std::vector<std::string>&, std::size_t, std::size_t);

std::vector<int> findAllOccurrences(std::string&, char);

template <class Iterable, typename it>
bool isIn(Iterable, it);

std::string splitString(std::string, char);

std::vector<std::string> splitIntoIterable(std::string string, char);

#endif