#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <functional>

class String {
public:
    std::string value;
    String(std::string val): value(val) {}
    String(const char* value): value(std::string(value)) {}
    String(): value("") {}

    std::string& operator+(const std::string& value) { return this->value += value; }
    
    bool operator!=(String value) { return this->value != (value.__to_cstr__()); }
    bool operator==(String value) { return this->value == (value.__to_cstr__()); }
    
    int length() { return this->value.length(); }
    int find(std::string value) { return this->value.find(value); }
    bool has(std::string value) { return this->value.find(value) != -1; }
    bool isEmpty() { return this->value.length() == 0; } 

    bool startsWith(std::string value) { return this->value.find(value) == 0; }
    bool endsWith(std::string value) { return this->value.find(value) == this->value.length() - value.length(); }
    bool isAscii() { return this->value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == std::string::npos; }
    bool isDigit() { return this->value.find_first_not_of("0123456789") == std::string::npos; }
    bool isAlpha() { return this->value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") == std::string::npos; }
    bool isAlphaNumeric() { return this->value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == std::string::npos; }
    bool isLowerCase() { return this->value.find_first_not_of("abcdefghijklmnopqrstuvwxyz") == std::string::npos; }
    bool isspace() { return this->value.find_first_not_of(" ") == std::string::npos; }

    const char* __rr() const { return "https://www.youtube.com/watch?v=dQw4w9WgXcQ"; }

    std::vector<std::string> split(std::string delitemer) 
    {
        std::vector<std::string> result;
        int start = 0;
        int end = 0;
        while (end != -1) 
        {
            end = this->value.find(delitemer, start);
            result.push_back(this->value.substr(start, end - start));
            start = end + delitemer.length();
        }
        return result;
    }


    std::string multiply(int value) 
    {
        for (int i = 0; i < value; i++) 
        {
            this->value += this->value;
        }
        return this->value;
    }

    std::string replace(std::string& value, std::string& replace) 
    {
        int start = 0;
        int end = 0;
        while (end != -1) 
        {
            end = this->value.find(value, start);
            this->value.replace(start, end - start, replace);
            start = end + replace.length();
        }
        return this->value;
    }

    template<typename T>
    std::string join(std::vector<T> iterable) 
    {
        std::string result;
        for (auto& value: iterable) {
            result += value;
        }

        return result;
    }

    std::string strip() 
    {
        this->value.erase(0, this->value.find_first_not_of(" "));
        this->value.erase(this->value.find_last_not_of(" ") + 1);
        return this->value;
    }

    int count(std::string& value) 
    {
        int count = 0;
        int start = 0;
        int end = 0;
        while (end != -1) 
        {
            end = this->value.find(value, start);
            count++;
            start = end + value.length();
        }
        return count;
    }
    
    std::string reverse() {
        std::string result;
        for (int i = this->value.length() - 1; i >= 0; i--) {
            result += this->value[i];
        }
        return result;
    }

    int __len__() { return this->value.length(); }
    const char* __to_cstr__() { return this->value.c_str(); }
    std::string __to_cpp_str__() const { return this->value; }
};
