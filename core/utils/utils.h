#include <iostream>
#include <vector>

#include "../lambda-code.h"

#ifndef UTILS_H_LAMBDA_CODE
#define UTILS_H_LAMBDA_CODE

struct F_IO_Object
{
    class R_ModeObject {
    private:
        LambdaCode::string source;
    public:
        R_ModeObject(LambdaCode::string source) : source(source) {}

        LambdaCode::string read() { return source; }
        std::vector<LambdaCode::string> readlines() {
            return source.split("\n");
        }
    };

    class W_ModeObject {
    private:
        std::string filePath;
        std::ofstream w_buf;

    public:
        W_ModeObject(LambdaCode::string filePath) : filePath(filePath.__to_cpp_str__()) {}

        void write(LambdaCode::string str, int streamCount = 0) {
            w_buf = std::ofstream(this->filePath);
            w_buf.write(str.__to_cstr__(), streamCount);
        }

        void close() { w_buf.close(); }
    };

    class DualModeObject {
    private:
        std::string filePath;
        std::ofstream w_buf;

    public:
        DualModeObject(LambdaCode::string filePath) : filePath(filePath.__to_cpp_str__()) {}

        void write(LambdaCode::string str, int streamCount = 0) {
            w_buf = std::ofstream(this->filePath, std::ios_base::app);
            w_buf.write(str.__to_cstr__(), streamCount);
        }

        LambdaCode::string read() {
            std::ifstream r_buf(this->filePath);
            std::string ret;
            std::string c_l;
            while (std::getline(r_buf, c_l))
                ret += c_l;
            return ret;
        }

        void close() { w_buf.close(); }
    };
};

auto range(int start, int end) {
    std::vector<int> v;
    for (int i = start; i < end; i++) 
        v.push_back(i);

    return v;
}

template <typename Indices>
bool isInsideString(std::string& source, std::string substr, Indices stringIndices) {
    throw std::runtime_error("Not implemented");
}

std::vector<int> findAllOccurrences(std::string& str, char substr) {
    std::vector<int> ret;
    int loop_count;
    for (loop_count = 0; loop_count < str.length(); loop_count++) 
        if (str[loop_count] == substr) 
            ret.push_back(loop_count);
        
    return ret;
}

std::string splitString(std::string string, char delimeter) {
    std::string ret;
    for (auto item : string) {
        if (item != delimeter) 
            ret += item;
        else 
            break;
        
    }
    return ret;
}

std::vector<std::string> splitIntoIterable(std::string string, char delimeter) {
    std::vector<std::string> ret;
    std::string temp;
    for (auto item : string) {
        if (item != delimeter) 
            temp += item;
        else {
            ret.push_back(temp);
            temp = "";
        }
    }
    ret.push_back(temp);
    return ret;
}

#endif