#pragma once
#include <vector>
#include <string>
#include <fstream>


#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

std::string getWorkingDirectory(const std::string& _path);

/**
 * @brief Get the working directory of the _path
 *
 * @param _path
 *
 * @return std::string
 */
std::string getWorkingDirectory(const std::string& _path);