#include <iostream>
#include <vector>
#include <initializer_list>
#include "../string/String.h"
#include "../utils/utils.h"

class Map {
public:
    Map(String data) {
        _checkMembership(data, '{');
    }

    ~Map() {

    }
};

int main() {

}