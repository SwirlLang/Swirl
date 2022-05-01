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
    Swirl_String(char *val) : value(val) {}
    Swirl_String(std::string val) : value((char *)val.c_str()) {}
    Swirl_String(const char *val) : value((char *)val) {}

    // bool operator!=(Swirl_String comp) { return comp.__to_cstr__() != value; }
    // bool operator==(Swirl_String comp) { return comp.__to_cstr__() == value; }

    Swirl_String operator+(Swirl_String str) { return strcat(value, str.__to_cstr__()); }
    Swirl_String operator*(int num) { return strncat(value, value, num); }

    bool operator==(Swirl_String comp) { return strcmp(value, comp.__to_cstr__()) == 0; }
    bool operator!=(Swirl_String comp) { return strcmp(value, comp.__to_cstr__()) != 0; }
    bool has(Swirl_String str) { return strstr(value, str.__to_cstr__()) != nullptr; }

    int length() { return strlen(value); }
    bool isEmpty() { return strlen(value) == 0; }

    std::vector<Swirl_String> split(Swirl_String chr)
    {
        std::vector<Swirl_String> result;
        char *str = value;
        char *token = strtok(str, chr.__to_cstr__());
        while (token != nullptr)
        {
            result.push_back(Swirl_String(token));
            token = strtok(nullptr, chr.__to_cstr__());
        }
        return result;
    }

    Swirl_String erase(Swirl_String str)
    {
        char *result = strstr(value, str.__to_cstr__());
        if (result != nullptr)
        {
            char *new_str = new char[strlen(value) - strlen(str.__to_cstr__()) + 1];
            strcpy(new_str, value);
            strcpy(new_str + (result - value), result + strlen(str.__to_cstr__()));
            return Swirl_String(new_str);
        }
        return Swirl_String(value);
    }

    int find(Swirl_String str)
    {
        for (int i = 0; i < str.length(); i++)
            if (value[i] == str.value[0])
                return i;
    }

    Swirl_String replace(Swirl_String before, Swirl_String after)
    {
        char *result = strstr(value, before.__to_cstr__());
        if (result != nullptr)
        {
            char *new_str = new char[strlen(value) - strlen(before.__to_cstr__()) + strlen(after.__to_cstr__()) + 1];
            strcpy(new_str, value);
            strcpy(new_str + (result - value), after.__to_cstr__());
            return Swirl_String(new_str);
        }
        return Swirl_String(value);
    }

    int toInt() { return atoi(value); }
    double toFloat() { return atof(value); }
    double toBool() { return static_cast<bool>(value); }

    const char *__rr__() { return ""; }
    const char *__to_cstr__() { return value; }
    std::string __to_cpp_str__() { return std::string(value); }
};

#endif
