//
// Created by owner on 10/21/2017.
//
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include "csv.h"
struct xyz {
    float x;
    float y;
    float z;
};
namespace csv {
    /**
     *
     * @return double of current time in seconds
     */
    long double getTime() {
        timeval time;
        gettimeofday(&time, 0);
        long double seconds;
        seconds = (long double)time.tv_sec;
        seconds += (time.tv_usec/(long double)1000000);
        return seconds;


    }
    void outputLine(xyz accel1, xyz accel2, xyz accel3) {

        long double timestamp; // Get current timestamp somehow, seconds.milliseconds
        std::string dateTime;
        std::ofstream file;
        file.open(dateTime, std::ios_base::app);
        if (file.is_open()) {
            file << std::to_string(timestamp) + "," +
                    std::to_string(accel1.x) + "," +
                    std::to_string(accel1.y) + "," +
                    std::to_string(accel1.z) + "," +
                    std::to_string(accel2.x) + "," +
                    std::to_string(accel2.y) + "," +
                    std::to_string(accel2.z) + "," +
                    std::to_string(accel3.x) + "," +
                    std::to_string(accel3.y) + "," +
                    std::to_string(accel3.z) + "," +
                    // Anything else to add can go in here
                    "\n";
        } else {
            std::cout << "\n\n FILE FAILED TO OPEN!";
            exit(5);
        }
    }

}