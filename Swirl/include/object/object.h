#include <vector>
#include <iostream>

#ifndef SWIRL_OBJECT_H
#define SWIRL_OBJECT_H

struct Object {
    bool is_builtin{};
    bool compile_alike{};
    bool call;

    std::string calls_to;
    std::vector<std::string> arguments{};

    Object() {
        call = is_builtin;
    }

    void clear() {
        is_builtin = false;
        compile_alike = false;
        call = false;
        calls_to.clear();
        arguments.clear();
    }
};

#endif
