#include <iostream>
#include <fstream>
#include "csv.cpp"


int main() {

    std::cout << "Time waiting";
    long double timeStart = csv::getTime();
    std::string s;
    std::cin >> s;
    long double timeEnd = csv::getTime();

    std::cout <<std::fixed<< "Time: " << (timeEnd-timeStart);
    return 0;
}