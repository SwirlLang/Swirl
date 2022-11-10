#include <iostream>

template < typename Const >
void print(Const __Obj, const std::string& __End = "\n", bool __Flush = true) {
    if (__Flush) std::cout << __Obj << __End << std::flush;
    else std::cout << __Obj << __End;
}
