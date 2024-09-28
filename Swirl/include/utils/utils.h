#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "llvm/IR/BasicBlock.h"


#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

/**
 * @brief Get the working directory of the _path
 *
 * @param _path
 *
 * @return std::string
 */
std::string getWorkingDirectory(const std::string& _path);