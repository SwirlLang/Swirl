#include <string>
#include <vector>
#include <filesystem>

#include <cli/cli.h>
#include <CompilerInst.h>
#include <include/SwirlConfig.h>


std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();


const std::vector<Argument> application_flags = {
    {{"-h","--help"}, "Show the help message", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-d", "--debug"}, "Log the steps of compilation", false, {}},
    {{"-v", "--version"}, "Show the version of Swirl", false, {}},
    {{"-t", "--threads"}, "No. of threads to use (excluding the main-thread).", true}
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


    if (!source_file_path.empty()) {
        if (!source_file_path.is_absolute())
            source_file_path = absolute(source_file_path);

        CompilerInst compiler_inst{source_file_path};
        if (app.contains_flag("-t"))
            compiler_inst.setBaseThreadCount(app.get_flag_value("-t"));

        compiler_inst.compile();
    }
}
