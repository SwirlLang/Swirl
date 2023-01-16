#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include <tokenizer/Tokenizer.h>

#ifndef _WIN32
#define TORNADO_PKGS_PATH "/.tornado/packages/"
#endif

void preProcess(const std::string& _source, TokenStream& _stream, std::string _buildPath) {
    std::stringstream source_strm(_source);
    std::vector<std::string> cimports{};

    std::string cr_dir = "cd " + _buildPath.erase(_buildPath.size() - 16, _buildPath.size())
            + "&& mkdir __swirl_cache__";

    if (!std::filesystem::exists(_buildPath + "__swirl_cache__"))
        system(cr_dir.c_str());

    std::ofstream cache_file(_buildPath + "__swirl_cache__" + PATH_SEP + "__main__.sw");
}
