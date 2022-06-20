#include <iostream>
#include <vector>
#include <fstream>

#include "swirl.typedefs/swirl_t.h"

struct F_IO_Object
{
    class R_ModeObject
    {
    private:
        Swirl::string source;

    public:
        R_ModeObject(Swirl::string source) : source(source) {}

        Swirl::string read() { return source; }
        std::vector<Swirl::string> readlines()
        {
            return source.split("\n");
        }
    };

    class W_ModeObject
    {
    private:
        std::string filePath;
        std::ofstream w_buf;

    public:
        W_ModeObject(Swirl::string filePath) : filePath(filePath.__to_cpp_str__()) {}

        void write(Swirl::string str, int streamCount = 0)
        {
            w_buf = std::ofstream(this->filePath);
            w_buf.write(str.__to_cstr__(), streamCount);
        }

        void close() { w_buf.close(); }
    };

    class DualModeObject
    {
    private:
        std::string filePath;
        std::ofstream w_buf;

    public:
        DualModeObject(Swirl::string filePath) : filePath(filePath.__to_cpp_str__()) {}

        void write(Swirl::string str, int streamCount = 0)
        {
            w_buf = std::ofstream(this->filePath, std::ios_base::app);
            w_buf.write(str.__to_cstr__(), streamCount);
        }

        Swirl::string read()
        {
            std::ifstream r_buf(this->filePath);
            std::string ret;
            std::string c_l;
            while (std::getline(r_buf, c_l))
                ret += c_l;
            return Swirl::string(static_cast<const char*>(ret.c_str()));
        }

        void close() { w_buf.close(); }
    };
};

std::string getPathSep() {
    std::string path_sep;
    #if defined(_WIN32)
        path_sep = "\\"
    #else
        path_sep = "/";
    #endif
    return path_sep;
}

bool isInString(std::size_t _Pos, std::string _Source) {
    // unimplemented
    return false;
}

std::string getWorkingDirectory(const std::string& _path) {
    return _path.substr(0, _path.find_last_of(getPathSep()));
}

std::vector<int> findAllOccurrences(std::string &str, char substr)
{
    std::vector<int> ret;
    int loop_count;
    for (loop_count = 0; loop_count < str.length(); loop_count++)
        if (str[loop_count] == substr)
            ret.push_back(loop_count);

    return ret;
}

std::string splitString(std::string string, char delimeter)
{
    std::string ret;
    for (auto item : string)
    {
        if (item != delimeter)
            ret += item;
        else
            break;
    }
    return ret;
}

std::vector<std::string> splitIntoIterable(std::string string, char delimeter)
{
    std::vector<std::string> ret;
    std::string temp;
    for (auto item : string)
    {
        if (item != delimeter)
            temp += item;
        else
        {
            ret.push_back(temp);
            temp = "";
        }
    }
    ret.push_back(temp);
    return ret;
}