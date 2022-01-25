#include <iostream>
#include <vector>
#include <chrono>
#include <memory>


#define print(x) std::cout << x << std::endl;


class ArrayOfString
{   
    ArrayOfString(std::initializer_list<std::string> args)
    {
        for (auto item: args)
        {
            print(item)
        }
    }
};



class String {

public:
    std::string value;

    String(std::string val = ""): value(val){};

    std::vector<std::string> split(std::string sep = " ")
    {
        std::vector<std::string> result = {};

        // TODO

        return result;
    }
    
};


int main(int argc, char* argv[]) 
{

    std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
    String* ello = new String("Hello world!");
    std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();

    String str("Hello world!");

}