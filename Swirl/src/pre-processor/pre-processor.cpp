#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>

#include <utils/utils.h>


void preProcess(std::string _source, std::string _exeFilePath, uint8_t& _exitCode) {
    std::istringstream source_str_buf(_source);
    std::string working_directory = getWorkingDirectory(_exeFilePath);
    std::string swirl_cache_path = working_directory + PATH_SEP + "__swirl_cache__" + PATH_SEP;
    std::vector<std::string> imports = {};
    std::vector<std::string> f_file_lines = {};

    for (std::string src_current_ln; std::getline(source_str_buf, src_current_ln);) {
        f_file_lines.push_back(src_current_ln);
        if (!src_current_ln.starts_with("import") && !isInString(src_current_ln.find("import"), src_current_ln) && !src_current_ln.empty())
            break;
        if (src_current_ln.find("import") != std::string::npos && !isInString(src_current_ln.find("import"), src_current_ln)) {
            std::string t_module = src_current_ln.substr(7, src_current_ln.size());
            replaceAll(t_module, PATH_SEP, ".");
            imports.push_back(t_module);
        }
    }

    try {
        std::filesystem::create_directory(std::string(working_directory) + PATH_SEP + "__swirl_cache__");
        // std::filesystem::create_directory(working_directory + PATH_SEP + "__swirl_cache__");
        std::ofstream cached_file(swirl_cache_path + "file_cache.sw");
        for (const std::string& f_ln : f_file_lines) {
            if (f_ln.find('\\') != std::string::npos && !isInString(f_ln.find('\\'), f_ln)) {
//                f_ln.replace()
            }
        }

        std::ofstream cached_modules_source(swirl_cache_path + "modules_cache.sw");

        for (const std::string &module : imports) {
            try {
                std::fstream _module(working_directory.append(PATH_SEP).append(module));
                std::string i_current_ln;
                while (std::getline(_module, i_current_ln)) {
                    std::cout << i_current_ln << std::endl;
                    _module << i_current_ln;
                }
            }
            catch (std::exception& spm_search_paths) {
                std::cout << "exception" << std::endl;
                // TODO implement looking for packages in the package manager's downloads directory
            }
        }
    } catch (std::exception& already_exist) {}
}
