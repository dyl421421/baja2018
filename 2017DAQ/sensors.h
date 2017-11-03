#ifndef SENSORS_H_
#define SENSORS_H_

#include <functional>
#include <tuple>

#include "accel.h"
#include "adc.h"
#include "ir_temp.h"

namespace sensors {
// Forwards to setup and tear down of dependencies
void init();
void begin();
void end();
void close();

// ADC

// Speed in mph
float front_right_hal();
float front_left_hal();
float rear_hal();
// Engine rpm
float rpm_tach();
// Pressure in psi
float front_brake_line_pressure();
float rear_brake_line_pressure();
// Streeing angle (in degrees)
float steering_angle();
// Suspension travel in inches
float front_left_suspension();
float front_right_suspension();
float rear_left_suspension();
float rear_right_suspension();
// Battery voltage
float battery_voltage();

// ACCEL

// Triple of (x, y, z) accel reading
std::tuple<float, float, float> accelXYZ();

// TEMP

// Temperature in Fahrenheit
float amb_temp();
float cvt_temp();
float rear_rotor_temp();
float front_left_rotor_temp();
float front_right_rotor_temp();

// GPIO

// true/false states
bool is_daq();
bool is_brake();

// Attach callback to shutdown button press
// Removes previously set callback
void on_shutdown(const std::function<void(void)> &callback);
}  // namespace sensors

#endif  // SENSORS_H_
