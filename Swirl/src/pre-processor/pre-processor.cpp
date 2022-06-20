#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>

#include <utils/utils.h>


void preProcess(std::string _source, std::string _exeFilePath, uint8_t& _exitCode) {
    std::istringstream source_str_buf(_source);
    std::string working_directory = getWorkingDirectory(_exeFilePath);
    std::string swirl_cache_path = working_directory + getPathSep() + "__swirl_cache__" + getPathSep();
    std::vector<std::string> imports = {};

    for (std::string src_current_ln; std::getline(source_str_buf, src_current_ln);) {
        if (!src_current_ln.starts_with("import") && !isInString(src_current_ln.find("import"), src_current_ln) && !src_current_ln.empty())
            break;
        if (src_current_ln.find("import") != std::string::npos && !isInString(src_current_ln.find("import"), src_current_ln)) {
            imports.push_back(src_current_ln.substr(7, src_current_ln.size()));
        }
    }

    try {
        std::filesystem::create_directory(std::string(working_directory) + getPathSep() + "__swirl_cache__");
        std::ofstream cached_file(swirl_cache_path + "file_cache.sw");
        cached_file << _source;
        cached_file.close();

        std::ofstream cached_modules_source(swirl_cache_path + "modules_cache.sw");

        for (const std::string& module : imports) {
            try {
                std::fstream _module(working_directory.append(getPathSep()).append(module));
                std::string i_current_ln;
                while (std::getline(_module, i_current_ln)) {
                    _module << i_current_ln;
                }
            }
            catch ( std::exception& spm_search_paths) {
                // TODO implement looking for packages in the package manager's downloads directory
            }
        }

    } catch (std::exception& already_exist) {}
}
