#include <iostream>
#include <vector>

#ifndef UTILS_H_LAMBDA_CODE
#define UTILS_H_LAMBDA_CODE

auto range(int start, int end) {
    std::vector<int> v;
    for (int i = start; i < end; i++) {
        v.push_back(i);
    }
    return v;
}



std::vector<int> findAllOccurrences(std::string& str, char substr) {
    std::vector<int> ret;
    int loop_count;
    for (loop_count = 0; loop_count < str.length(); loop_count++) {
        if (str[loop_count] == substr) {
            ret.push_back(loop_count);
        }
    }
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

std::vector<std::string> splitStringIntoList(std::string string, char delimeter) {
    std::vector<std::string> ret;
    std::string temp;
    for (auto item : string) {
        if (item != delimeter) {
            temp += item;
        } else {
            ret.push_back(temp);
            temp = "";
        }
    }
    ret.push_back(temp);
    return ret;
}

// template <typename Range>
// int returnOccurrence(std::string& string, Range arrayIndexes[], bool debug, const char chr) {
//     int ret = -1;
//     int loop_count;

//     if (((sizeof(arrayIndexes) / sizeof(int)) % 2) != 0) {
//         if (debug)
//             std::cout << "Error: arrayIndexes must be even!" << std::endl;
//     }

//     for (char letter : string) {
//         if (char == chr)
//             for (auto ranges : arrayIndexes) 
//                 for (auto index : ranges) 
//                     if (loop_count != index)
//                         return ret;               
            
        
//         loop_count += 1;
//     }

//     return ret;
// }

#endif