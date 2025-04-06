#include "utils/utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <cli/cli.h>
#include <include/SwirlConfig.h>
#include <parser/Parser.h>

std::string INPUT_FILE_PATH;
std::string OUTPUT_FILE_PATH;
std::string INPUT_FILE_CONTENT;

std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();
std::optional<ThreadPool_t> ThreadPool = std::nullopt;

const std::vector<Argument> application_flags = {
    {{"-h", "--help"}, "Show the help message", false, {}},
    {{"-o", "--output"}, "Output file name", true, {}},
    {{"-d", "--debug"}, "Log the steps of compilation", false, {}},
    {{"-v", "--version"}, "Show the version of Swirl", false, {}},
    {{"-t", "--threads"},
     "No. of threads to use (beside the mainthread).",
     true}};

int main(int argc, const char **argv) {
  cli app(argc, argv, application_flags);

  if (app.contains_flag("-h")) {
    std::cout << USAGE << app.generate_help() << '\n';
    return 0;
  }

  if (app.contains_flag("-v")) {
    std::cout << "Swirl v" << swirl_VERSION_MAJOR << "." << swirl_VERSION_MINOR
              << "." << swirl_VERSION_PATCH << "\n";
    return 0;
  }

  std::optional<std::string> _file = app.get_file();

  if (!_file.has_value()) {
    std::cerr << "No Input file\n";
    return 1;
  }

  INPUT_FILE_PATH = *app.get_file();

  if (!std::filesystem::exists(INPUT_FILE_PATH)) {
    std::cerr << "File '" << INPUT_FILE_PATH << "' not found!" << std::endl;
    return 1;
  }

  std::ifstream file_stream(INPUT_FILE_PATH);
  INPUT_FILE_CONTENT = {std::istreambuf_iterator(file_stream),
                        std::istreambuf_iterator<char>{}};
  file_stream.close();

  std::string file_name =
      INPUT_FILE_PATH.substr(INPUT_FILE_PATH.find_last_of("/\\") + 1);
  std::string out_dir = INPUT_FILE_PATH.substr().replace(
      INPUT_FILE_PATH.find(file_name), file_name.length(), "");
  file_name = file_name.substr(0, file_name.find_last_of('.'));

  if (app.contains_flag("-o"))
    OUTPUT_FILE_PATH = app.get_flag_value("-o");
  else {
    OUTPUT_FILE_PATH = file_name;
  }

  if (app.contains_flag("-j")) {
    ThreadPool.emplace(std::stoi(app.get_flag_value("-j")));
  } else {
    ThreadPool.emplace(std::thread::hardware_concurrency() / 2);
  }

  INPUT_FILE_CONTENT += "\n";

  if (!INPUT_FILE_CONTENT.empty()) {
    Parser parser(INPUT_FILE_PATH);
    parser.parse();
    parser.callBackend();
  }

  ThreadPool->shutdown();
}
