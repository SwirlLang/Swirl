#include <getopt.h>
#include <iostream>
#include <string>

#define has_value "1"
#define no_value "0"

struct arg {
    const char* name;
    const char* description;
    const char* val;
    bool required;
    std::vector<const char*> flags;
};

std::vector<arg> args_list = {
    {"help", "Show help message", no_value, false, {"-h", "--help"}},
    {"debug", "Log out the compilation steps", no_value, false, {"-d", "--debug"}},
    {"output", "Output file name", has_value, false, {"-o", "--output"}},
    {"run", "Run the compiled file", no_value, false, {"-r", "--run"}},
    {"compiler", "C++ compiler to use", has_value, false, {"-c", "--compiler"}},
};

static struct option *long_options = new struct option[args_list.size()];
std::string optstring = ":";

// static struct option long_options2[] = {
//         {"output", required_argument, 0, 'o'},
//         {"verbose", no_argument, 0, 'v'},
// };


void parse(int argc, char* argv[]) {
    for (int i = 0; i < args_list.size(); i++) {
        long_options[i].name = args_list[i].name;
        long_options[i].has_arg = (args_list[i].val != "1") ? no_argument : required_argument;
        long_options[i].flag = nullptr;
        long_options[i].val = args_list[i].flags[0][1];
    }
    // create the argstring
    for (int i = 0; i < args_list.size(); i++) {
        optstring += args_list[i].flags[0][1];
        if (args_list[i].val == has_value) {
            optstring += ":";
        }
    }

    int opt;
    while ((opt = getopt_long(argc, argv, optstring.c_str(), long_options, NULL)) != -1) {
        switch (opt) {
            case ':':
                std::cout << "Option: " << (char)optopt << " requires an argument" << std::endl;
                break;
            case '?':
                std::cerr << "Usage: " << argv[0] << " [-o value -v] file" << std::endl;
                break;
            default:
                for (int i = 0; i < args_list.size(); i++) {
                    if (args_list[i].flags[0][1] == opt) {
                        // check if the option requires a value
                        if (args_list[i].val == has_value) {
                            std::cout << args_list[i].name << ": " << optarg << std::endl;
                            args_list[i].val = optarg;
                        } else {
                            std::cout << args_list[i].name << ": " << "true" << std::endl;
                        }
                        break;
                    }
                }
                break;
        }
    }
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-o value -v] file" << std::endl;
    }
    delete[] long_options;
}
