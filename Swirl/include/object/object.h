#include <vector>
#include <iostream>
#include <memory>

#ifndef SWIRL_OBJECT_H
#define SWIRL_OBJECT_H

struct Object {};

enum TYPE {
    CALL, CONDITION, CLASS, FOR_LOOP,
    WHILE_LOOP, FUNCTION
};

struct Var : Object {
    const char* type{};
    const char* ident{};
    const char* value{};
};

struct Function : Object{
    TYPE type = TYPE::FUNCTION;
    const char* id;
    std::vector<Var> params;
};

struct Call : Object{
    const char* id{};
    std::vector<const char*> args{};
};

struct AbstractSyntaxTree {
    std::unique_ptr<std::vector<Object>> children = std::make_unique<std::vector<Object>>(std::vector<Object>());
};

#endif
