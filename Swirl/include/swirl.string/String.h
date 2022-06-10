#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#ifndef Swirl_STRING_H
#define Swirl_STRING_H

class Swirl_String
{
public:
    char *value;
    Swirl_String(char *val);
    Swirl_String(std::string val);
    Swirl_String(const char *val);
    Swirl_String operator+(Swirl_String str);
    Swirl_String operator*(int num);

    bool operator==(Swirl_String comp);
    bool operator!=(Swirl_String comp);
    bool has(Swirl_String str);

    int length();
    bool isEmpty();
    std::vector<Swirl_String> split(Swirl_String chr);

    Swirl_String erase(Swirl_String str);

    int find(Swirl_String str);

    Swirl_String replace(Swirl_String before, Swirl_String after);
    int toInt();
    double toFloat();
    double toBool();

    const char *__rr__();
    const char *__to_cstr__();
    std::string __to_cpp_str__();
};

#endif
