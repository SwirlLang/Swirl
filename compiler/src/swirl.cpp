#include <string>
#include <vector>
#include <filesystem>

#include <cli/cli.h>
#include <CompilerInst.h>
#include <include/SwirlConfig.h>

const std::vector<Argument> application_flags = {
    {{"-h", "--help"}, "Show the help message", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-v", "--version"}, "Show the version of Swirl", false, {}},
    {{"-j", "--threads"}, "No. of threads to use (excluding the main-thread).", true},
    {{"-t", "--target"}, "The target-triple of the target-platform", true},
    {{"-l", "--library"}, "The name of the library to link against.", true, true},
    {{"-p", "--project"}, "/path/to/project/root", true},
    {{"-d", "--dependency"}, "Register a dependency, in the format `path:name`.", true, true},
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
        if (app.contains_flag("-l")) {
            for (auto lib : app.get_flag_values("-l"))
                CompilerInst::appendLinkTarget(lib);
        }
        if (app.contains_flag("-d")) {
            // format: `path:alias`
            for (auto dep : app.get_flag_values("-d"))
                CompilerInst::addPackageEntry(dep);
        }
        if (app.contains_flag("-p"))
            CompilerInst::addPackageEntry(
                app.get_flag_value("-p") + ':' + fs::path(app.get_flag_value("-p")).filename().string(), true);
        else {
            // if `-p` isn't passed explicitly, assume that the project root is the
            // directory in which the source file resides
            CompilerInst::addPackageEntry(source_file_path.parent_path().string() + ':' +
                                              source_file_path.parent_path().filename().string(),
                                          true);
        }

        compiler_inst.compile();
    }
}
