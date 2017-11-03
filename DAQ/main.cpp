#include <iostream>
#include <wiringPiI2C.h>
#include "csv.h"


int main() {
	csv::outputLine({1,2,3},{1,2,3},{1,2,3});
	return 0;
};
