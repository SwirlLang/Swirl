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

#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

/**
 * @brief Replace the _from string in the _source string to the _to string
 *
 * @param _source the string to replace the _from string in
 * @param _from the string to replace
 * @param _to the string to replace _from with
 *
 * @return void
 */
void replaceAll(std::string &_source, std::string _from, std::string _to);

/**
 * @brief Check if the _val is in the _iter
 *
 * @tparam _iter
 * @param _val
 *
 * @return auto
 */
template <class Iterable, typename Any>
auto isIn(Iterable _iter, Any _val);

// std::vector<std::string> sliceVec(std::vector<std::string>&, std::size_t, std::size_t);

/**
 * @brief Get the index of the _item in the _vec
 *
 * @tparam T
 * @param _vec
 * @param _item
 *
 * @return std::size_t
 */
template <class Type, typename T>
std::size_t getIndex(std::vector<Type> _vec, T _item);

/**
 * @brief UNIMPLEMENTED. Check if the _pos is in the _source
 *
 * @param _pos
 * @param _source
 *
 * @return bool
 */
bool isInString(std::size_t _pos, std::string _source);

/**
 * @brief Get the working directory of the _path
 *
 * @param _path
 *
 * @return std::string
 */
std::string getWorkingDirectory(const std::string& _path);

/**
 * @brief Get all the occurrences of the _substr in the _str
 *
 * @param _str
 * @param _substr
 *
 * @return std::vector<int>
 */
std::vector<int> findAllOccurrences(std::string& _str, char _substr);

/**
 * @brief Split the string when the _delimiter is found
 *
 * @param _str
 * @param _delimiter
 *
 * @return std::string
 */
std::string splitString(std::string _str, char _delimiter);

/**
 * @brief Split the string when the _delimiter is found into a vector
 *
 * @param _str
 * @param _delimiter
 *
 * @return std::vector<std::string>
 */
std::vector<std::string> splitIntoIterable(std::string _str, char _delimeter);

// template <typename Indices>
// bool isInsideString(std::string &source, std::string substr, Indices stringIndices);

#endif