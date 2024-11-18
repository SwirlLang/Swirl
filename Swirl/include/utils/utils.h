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


struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator ()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ (h2 << 1);
    }
};
