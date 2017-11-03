#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <mutex>

namespace display {

const unsigned int STATUS_NONE        = 0;
const unsigned int WARNING_BATTERY    = 1 << 0;
const unsigned int WARNING_TEMP       = 1 << 1;
const unsigned int INFO_BRAKE         = 1 << 2;
const unsigned int INFO_DATA_LOGGING  = 1 << 3;
const unsigned int STATUS_UNKNOWN     = 1 << 4;

// Initializes connection with pigpio daemon
// Should be called once before begin
void init();

// Sets up pins and turns display state off
// Should be called once before update
void begin();

// Update method updates the display with new information
// rpm the rpm to update, must be positive
// mph the mph to update, must be positive
// status_flags use combination of constants defined above such as
//               WARNING_TEMP | INFO_BRAKE
void update(unsigned int rpm, unsigned int mph, unsigned int status_flags);

// Ensures display state is off
// Should be called before close
void end();

// Closes gpio access
// NOT THREAD-SAFE (Should be called after all threads are synchronized,
// if at all).
void close();

}  // namespace display

#endif  // DISPLAY_H_
