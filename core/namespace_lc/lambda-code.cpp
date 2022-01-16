#include <iostream>
#include <cstring>
#include <vector>


// for debugging purposes only!
#define print(x) std::cout << x << std::endl;



namespace __LambdaCode__ {
    class string: public std::string {
        public:
            std::string value;

            string() {
                this -> value = nullptr;
            }

            string(std::string& val): value(val){};

            auto split(std::string string, std::string sep = " ") {
                std::string array = {};

                for (int i = 0; i < sizeof(string); i++) {
                    
                }
            }
    };
}


int main() {

}