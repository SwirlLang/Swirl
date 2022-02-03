#include <iostream>
#include <string>
#include "string.h"

class String {
private:
    template<typename _Any>
    bool _checkMembership(_Any member, _Any array) {
        for (auto iter : array)
        {
            if (iter == *member) return true;
            else
                return false;
        }
    }

    // template<typename _Any>
    // int _getIndex(_Any member, _Any array) {
    //     int loop_count = 0;

    //     for (auto iter : array) {
    //         if (loop_count != 0) {

    //         }
    //     }
    // }

public:
    std::string value;

    String(std::string value): value(value){};
    String();

    std::string upper() {
        std::string ret = "";
        std::string case_map[2][26] = {
                {
                    "a", "b", "c", "d", "e", "f", "g", "h",
                    "i", "j", "k", "l", "m", "n", "o", "p",
                    "q", "r", "s", "t", "u", "v", "w", "x",
                    "y", "z"
                    },

                    {
                    "A", "B", "C", "D", "E", "F", "G", "H",
                    "I", "J", "K", "L", "M", "N", "O", "P",
                    "Q", "R", "S", "T", "U", "V", "W", "X",
                    "Y", "Z"
                    }
        };

        for (int chr_i = 0; chr_i < value.length(); chr_i++) {
            if (_checkMembership((std::string*)value[chr_i], case_map[0])) {
                // TODO
            }
        }

        return ret;
    }
};


using std::cout;

template<typename _Any>
int _getIndex(_Any member = nullptr, _Any array = nullptr) {
    int loop_count = 0;
    auto count = []() {loop_count != 0 ? loop_count += 1 : return};
    std::for_each(array.begin(), array.end(), count);
}

int main()
{
    std::string e[] = {"Hello World!"};
    //_getIndex();
}