#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#ifndef Swirl_STRING_H
#define Swirl_STRING_H

class LC_String
{
public:
    char *value;
    LC_String(char *val) : value(val) {}
    LC_String(std::string val) : value((char *)val.c_str()) {}
    LC_String(const char *val) : value((char *)val) {}

    // bool operator!=(LC_String comp) { return comp.__to_cstr__() != value; }
    // bool operator==(LC_String comp) { return comp.__to_cstr__() == value; }

    LC_String operator+(LC_String str) { return strcat(value, str.__to_cstr__()); }
    LC_String operator*(int num) { return strncat(value, value, num); }

    bool operator==(LC_String comp) { return strcmp(value, comp.__to_cstr__()) == 0; }
    bool operator!=(LC_String comp) { return strcmp(value, comp.__to_cstr__()) != 0; }
    bool has(LC_String str) { return strstr(value, str.__to_cstr__()) != nullptr; }

    int length() { return strlen(value); }
    bool isEmpty() { return strlen(value) == 0; }

    std::vector<LC_String> split(LC_String chr)
    {
        std::vector<LC_String> result;
        char *str = value;
        char *token = strtok(str, chr.__to_cstr__());
        while (token != nullptr)
        {
            result.push_back(LC_String(token));
            token = strtok(nullptr, chr.__to_cstr__());
        }
        return result;
    }

    LC_String erase(LC_String str)
    {
        char *result = strstr(value, str.__to_cstr__());
        if (result != nullptr)
        {
            char *new_str = new char[strlen(value) - strlen(str.__to_cstr__()) + 1];
            strcpy(new_str, value);
            strcpy(new_str + (result - value), result + strlen(str.__to_cstr__()));
            return LC_String(new_str);
        }
        return LC_String(value);
    }

    int find(LC_String str)
    {
        for (int i = 0; i < str.length(); i++)
            if (value[i] == str.value[0])
                return i;
    }

    LC_String replace(LC_String before, LC_String after)
    {
        char *result = strstr(value, before.__to_cstr__());
        if (result != nullptr)
        {
            char *new_str = new char[strlen(value) - strlen(before.__to_cstr__()) + strlen(after.__to_cstr__()) + 1];
            strcpy(new_str, value);
            strcpy(new_str + (result - value), after.__to_cstr__());
            return LC_String(new_str);
        }
        return LC_String(value);
    }

    int toInt() { return atoi(value); }
    double toFloat() { return atof(value); }
    double toBool() { return static_cast<bool>(value); }

    const char *__rr__() { return ""; }
    const char *__to_cstr__() { return value; }
    std::string __to_cpp_str__() { return std::string(value); }
};

#endif
