#include "utils/utils.h"
#include <string>
#include <vector>
#include <thread>
#include <filesystem>

#include <cli/cli.h>
#include <parser/Parser.h>
#include <include/SwirlConfig.h>


std::filesystem::path SW_OUTPUT;


std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();
std::optional<ThreadPool_t> ThreadPool = std::nullopt;


const std::vector<Argument> application_flags = {
    {{"-h","--help"}, "Show the help message", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-d", "--debug"}, "Log the steps of compilation", false, {}},
    {{"-v", "--version"}, "Show the version of Swirl", false, {}},
    {{"-t", "--threads"}, "No. of threads to use (besides the mainthread).", true}
};


int main(int argc, const char** argv) {
    cli app(argc, argv, application_flags);

    if (app.contains_flag("-h")) {
        std::println("{}{}", USAGE, app.generate_help());
        return 0;
    }

    if (app.contains_flag("-v")) {
        std::println("Swirl v{}.{}.{}, built on {}.", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, __DATE__);
        return 0;
    }

    std::optional<std::string> _file = app.get_file();

    if (!_file.has_value()) {
        std::println(stderr, "No input file");
        return 1;
    }

    std::filesystem::path source_file_path = *app.get_file();

    if (!exists(source_file_path)) {
        std::println(stderr, "File \"{}\" not found!", source_file_path.string());
        return 1;
    }


    auto out_dir = source_file_path.parent_path();
    auto file_name = source_file_path.filename();

    
    if (app.contains_flag("-o"))
        SW_OUTPUT = app.get_flag_value("-o");
    else { SW_OUTPUT = file_name; }


    if (app.contains_flag("-j")) {
        ThreadPool.emplace(std::stoi(app.get_flag_value("-j")));
    } else { ThreadPool.emplace(std::thread::hardware_concurrency() / 2); }


    if (!source_file_path.empty()) {
        if (!source_file_path.is_absolute()) {
            source_file_path = absolute(source_file_path);
        }

        Parser parser(source_file_path);
        parser.parse();
        parser.callBackend();
    }

    ThreadPool->shutdown();
}
