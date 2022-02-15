#include <iostream>
#include <string>
#include <cstring>
#include <optional>
#include <vector>

class String {
private:
    std::string value;

public:
    String(std::string value): value(value) {}
    String(): value(nullptr) {}

    std::string& operator+(const std::string& value) { return this->value += value; }
    
    bool operator!=(const std::string& value) { return this->value != value; }
    bool operator==(const std::string& value) { return this->value == value; }

    std::vector<std::string> split(std::string delitemer) {
        std::vector<std::string> result;
        int start = 0;
        int end = 0;
        while (end != -1) {
            end = this->value.find(delitemer, start);
            result.push_back(this->value.substr(start, end - start));
            start = end + delitemer.length();
        }
        return result;
    }

    int length() { return this->value.length(); }
    int find(std::string value) { return this->value.find(value); }
    bool has(std::string value) { return this->value.find(value) != -1; }
    bool isEmpty() { return this->value.length() == 0; } 

    std::string& multiply(int value) {
        for (int i = 0; i < value; i++) {
            this->value += this->value;
        }
        return this->value;
    }
};