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

struct pos_arg {
    const char* name;
    const char* description;
    const char* val;
};

std::string generate_help(std::vector<arg> args_list) {
    std::string help = "The Swirl compiler\n\nUsage: Swirl [flags] file";

    help += "\n\nFlags:\n";
    // generate the flags
    for (auto& arg : args_list) {
        help+="\t";
        for (auto& flag : arg.flags) {
            // check if it isn't the last flag
            if (flag != arg.flags.back()) {
                help += std::string(flag) + ", ";
            } else {
                help += std::string(flag);
            }
        }
        help += "\t" + std::string(arg.description) + "\n";
    }
    return help;
}

std::string optstring = ":";

void parse(int argc, char* argv[], std::vector<arg>& args_list, std::vector<pos_arg>& pos_args_list) {
    static struct option *long_options = new struct option[args_list.size()];
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
                // get the name of the flag
                for (auto& arg : args_list) {
                    for (auto& flag : arg.flags) {
                        if (flag[1] == optopt) {
                            std::cerr << "Error: " << arg.name << " flag requires a value \n";
                        }
                    }
                }
                break;
            case '?':
                std::cerr << "Usage: "<< argv[0] << " [flags] file" << std::endl;
                break;
            default:
                for (int i = 0; i < args_list.size(); i++) {
                    if (args_list[i].flags[0][1] == opt) {
                        // check if the option requires a value
                        if (args_list[i].val == has_value) {
                            args_list[i].val = optarg;
                        } else {
                            args_list[i].val = "true";
                        }
                        break;
                    }
                }
                break;
        }
    }
    if (argc < 2) {
        std::cerr << "Usage: "<< argv[0] << " [flags] file" << std::endl;
        exit(0);
    }

    // assign the positional arguments
    int pos_arg_index = 0;
    for (int i = optind; i < argc; i++) {
        pos_args_list[pos_arg_index].val = argv[i];
        pos_arg_index++;
    }

    // check if -h or --help is passed
    if (args_list[0].val == "true") {
        std::cout << generate_help(args_list) << std::endl;
        exit(0);
    }

    // print the positional args
    for (int i = 0; i < pos_args_list.size(); i++) {
        if (pos_args_list[i].val == "0") {
            std::cerr << "Usage: "<< argv[0] << " [flags] file" << std::endl;
        }
    }
    delete[] long_options;
}
