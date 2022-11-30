#include <iostream>

#define and &&
#define not !
#define or ||

template < typename Const >
void print(Const __Obj, const std::string& __End = "\n", bool __Flush = true) {
    if (__Flush) std::cout << __Obj << __End << std::flush;
    else std::cout << __Obj << __End;
}

std::string input(std::string __Prompt) {
    std::string ret;
    std::cout << __Prompt << std::flush;
    std::getline(std::cin, ret);

    return ret;
}
