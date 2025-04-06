#include "utils/utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>

#include <cli/cli.h>
#include <parser/Parser.h>
#include <include/SwirlConfig.h>


std::string SW_FED_FILE_PATH;
std::string SW_OUTPUT;
std::string SW_FED_FILE_SOURCE;


std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();
std::optional<ThreadPool_t> ThreadPool = std::nullopt;

const std::vector<Argument> application_flags = {
    {{"-h","--help"}, "Show the help message", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-d", "--debug"}, "Log the steps of compilation", false, {}},
    {{"-v", "--version"}, "Show the version of Swirl", false, {}},
    {{"-t", "--threads"}, "No. of threads to use (beside the mainthread).", true}
};


int main(int argc, const char** argv) {
    cli app(argc, argv, application_flags);

    if (app.contains_flag("-h")) {
        std::cout << USAGE << app.generate_help() << '\n';
        return 0;
    }

    if (app.contains_flag("-v")) {
        std::cout << "Swirl v" << swirl_VERSION_MAJOR << "." << swirl_VERSION_MINOR << "." << swirl_VERSION_PATCH << "\n";
        return 0;
    }

    std::optional<std::string> _file = app.get_file();

    if (!_file.has_value()) {
        std::cerr << "No Input file\n";
        return 1;
    }

    SW_FED_FILE_PATH = *app.get_file();

    if (!std::filesystem::exists(SW_FED_FILE_PATH)) {
        std::cerr << "File '" << SW_FED_FILE_PATH << "' not found!" << std::endl;
        return 1;
    }

    std::ifstream file_stream(SW_FED_FILE_PATH);
    SW_FED_FILE_SOURCE = {std::istreambuf_iterator(file_stream), std::istreambuf_iterator<char>{}};
    file_stream.close();

    std::string file_name = SW_FED_FILE_PATH.substr(SW_FED_FILE_PATH.find_last_of("/\\") + 1);
    std::string out_dir = SW_FED_FILE_PATH.substr().replace(SW_FED_FILE_PATH.find(file_name),file_name.length(),"");
    file_name = file_name.substr(0, file_name.find_last_of('.'));

    SW_OUTPUT = file_name;

    if (app.contains_flag("-o"))
        SW_OUTPUT = app.get_flag_value("-o");

    if (app.contains_flag("-j")) {
        ThreadPool.emplace(std::stoi(app.get_flag_value("-j")));
    } else { ThreadPool.emplace(std::thread::hardware_concurrency() / 2); }

    SW_FED_FILE_SOURCE += "\n";

    if ( !SW_FED_FILE_SOURCE.empty() ) {
        Parser parser(SW_FED_FILE_PATH);
        parser.parse();
        parser.callBackend();
    }

    ThreadPool->shutdown();
}
