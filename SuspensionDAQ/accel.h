#ifndef ACCEL_H_
#define ACCEL_H_

#include <cmath>
#include <cstdint>

namespace accel {
// Read Status
enum Status {
  OK,
  BAD_RETURN_LENGTH,
};

// Connect to daemon
void init();
// Pin setup and I2C configuration etc.
void begin();
// Release pins and I2C
void end();
// Disconnect from daemon
void close();

// Acceleration readings for all 3 axes
struct AccReading { double x, y, z; Status stat; };
AccReading get_acceleration();

// 16-bit ADC readings for all 3 inputs
struct AdcReading { uint16_t out1, out2, out3; Status stat; };
AdcReading get_adc();

// Interal temperature reading
struct TempReading { double temp; Status stat; };
TempReading get_temperature();
}  // namespace accel

#endif  // ACCEL_H_
