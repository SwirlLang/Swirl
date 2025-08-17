#include <string>
#include <vector>
#include <filesystem>

#include <cli/cli.h>
#include <CompilerInst.h>
#include <include/SwirlConfig.h>


const std::vector<Argument> application_flags = {
    {{"-h","--help"}, "Show the help message", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-v", "--version"}, "Show the version of Swirl", false, {}},
    {{"-j", "--threads"}, "No. of threads to use (excluding the main-thread).", true},
    {{"-t", "--target"}, "The target-triple of the target-platform", true},
    {{"-l", "--library"}, "The name of the library to link against.", true},
    {{"-p", "--project"}, "/path/to/project/root", true},
    {{"-d", "--dependency"}, "Register a dependency, in the format `path:name`.", true},
    {{"-Ld", "--debug"}, "Log the steps of compilation", false, {}},
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

        if (app.contains_flag("-j"))
            compiler_inst.setBaseThreadCount(app.get_flag_value("-j"));
        if (app.contains_flag("-t"))
            CompilerInst::setTargetTriple(app.get_flag_value("-t"));
        if (app.contains_flag("-l"))  // until the cli supports multiple-values for the same flag...
            CompilerInst::appendLinkTarget(app.get_flag_value("-l"));
        if (app.contains_flag("-d")) { // until the cli supports multiple-values for the same flag...
            // format: `path:alias`
            auto flag_val = app.get_flag_value("-d");
            auto alias = flag_val.substr(flag_val.find_last_of(':') + 1);
            auto path = flag_val.erase(flag_val.find_last_of(':'));

            CompilerInst::PackageTable.insert({alias, path});
        }
        if (app.contains_flag("-p"))
            CompilerInst::PackageTable.insert({
                fs::path(app.get_flag_value("-p")).filename().string(),
                fs::path(app.get_flag_value("-p"))
            });
        else {
            // if `-p` isn't passed explicitly, assume that the project root is the
            // directory in which the source file resides
            CompilerInst::PackageTable.insert({
                source_file_path.parent_path().filename().string(),
                source_file_path.parent_path()
            });
        }


        compiler_inst.compile();
    }
}
