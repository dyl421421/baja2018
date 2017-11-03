#ifndef IR_TEMP_
#define IR_TEMP_

#include <cmath>

namespace ir_temp {
enum Device {
  CVT_BELT,
  R_ROTOR,
  FL_ROTOR,
  FR_ROTOR,
  NUM_DEVICES,    // Must be the last device in the list
};

enum Status {
  OK,
  BAD_RETURN_LEN,
  CRC8_MISMATCH,
  BAD_HANDLE,
};

// Connect to pigpio daemon
void init();
// Pin setup and other settings
void begin();
// Release pins
void end();
// Disconnect from daemon
void close();

// Get temperatures from a device
struct Reading { double val; Status stat = OK; };
Reading get_obj(Device d);
Reading get_amb(Device d);
}  // namespace ir_temp

#endif  // IR_TEMP_
