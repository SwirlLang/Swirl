#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>

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
            return {static_cast<const char*>(ret.c_str())};
        }

        void close() { w_buf.close(); }
    };
};

#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

void replaceAll(std::string& _source, std::string _from, std::string _to) {
    std::size_t pos = _source.find(_from);
    while( pos != std::string::npos)
    {
        _source.replace(pos, _from.size(), _to);
        pos = _source.find(_from, pos + _to.size());
    }
}

template <class Iterable, typename Any>
auto isIn(Iterable _iter, Any _val) {
    return std::find(_iter.begin(), _iter.end(), _val);
}

//std::vector<std::string> sliceVec(std::vector<std::string>& _vec, std::size_t _m, std::size_t _n ) {
//    std::vector<std::string> ret;
//    auto vec_start = _vec.begin() + _m;
//    auto vec_end = _vec.end() + _n + 1;
//    ret(_m - _n + 1);
//    copy(vec_start, vec_end, ret.begin());
//
//    return ret;
//}

template <class Type, typename T>
std::size_t getIndex(std::vector<Type> _vec, T _item) {
    std::size_t index = 0;
    for (auto const& item : _vec) {
        if (item == _item)
            return index;
        index += 1;
    }
    return -1;
}

bool isInString(std::size_t _pos, std::string _source) {
    // unimplemented
    return false;
}

std::string getWorkingDirectory(const std::string& _path) {
    return _path.substr(0, _path.find_last_of(PATH_SEP));
}

std::vector<int> findAllOccurrences(std::string& _str, char _substr)
{
    std::vector<int> ret;
    int loop_count;
    for (loop_count = 0; loop_count < _str.length(); loop_count++)
        if (_str[loop_count] == _substr)
            ret.push_back(loop_count);

    return ret;
}

std::string splitString(std::string _str, char _delimiter)
{
    std::string ret;
    for (auto item : _str)
    {
        if (item != _delimiter)
            ret += item;
        else
            break;
    }
    return ret;
}

std::vector<std::string> splitIntoIterable(std::string _str, char _delimeter)
{
    std::vector<std::string> ret;
    std::string temp;
    for (auto item : _str)
    {
        if (item != _delimeter)
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