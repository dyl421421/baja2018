/*
 * accel.h
 *
 *  Created on: Oct 28, 2017
 *      Author: owner
 */

#ifndef ACCEL_H_
#define ACCEL_H_

#include <wiringPiI2C.h>

struct xyz {
	double x;
	double y;
	double z;
};
namespace {

	xyz readVector(int pin);
	double readVoltage(int pin);
	double readTemp(int pin);



}
#endif /* ACCEL_H_ */
