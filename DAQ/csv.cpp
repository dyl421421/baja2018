//
// Created by owner on 10/21/2017.
//
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <string>
#include "csv.h"
#define FILENAME "temp1"
namespace csv {
    /**
     *
     * @return double of current time in seconds
     */
    double getTime() {
        timeval time;
        gettimeofday(&time, 0);
        long double seconds;
        seconds = (long double)time.tv_sec;
        seconds += (time.tv_usec/(long double)1000000);
        return seconds;


    }
    void outputLine(xyz accel1, xyz accel2, xyz accel3) {

        double timestamp = csv::getTime(); // Get current timestamp somehow, seconds.milliseconds
        char *dateTime = "temp";
        std::ofstream file;
        file.open(dateTime, std::ios_base::app);
        if (file.is_open()) {
            std::string line;
            std::string comma = ",";
        	line =	(timestamp) + comma +
                    (accel1.x) + comma +
                    (accel1.y) + comma +
                    (accel1.z) + comma +
                    (accel2.x) + comma +
                    (accel2.y) + comma +
                    (accel2.z) + comma +
                    (accel3.x) + comma +
                    (accel3.y) + comma +
                    (accel3.z) + comma +
                    // Anything else to add can go in here
                    "\n";
        } else {
            std::cout << "\n\n FILE FAILED TO OPEN!";
        }
    }

}
