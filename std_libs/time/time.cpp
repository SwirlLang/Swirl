#include <iostream>
#include <ctime>
using namespace std;

namespace time{
    void currentTime(){
        char* currentTime(){
        time_t now = time(0);

        // convert now to string form
       char* date_time = ctime(&now);

       return date_time;

};
    }
}