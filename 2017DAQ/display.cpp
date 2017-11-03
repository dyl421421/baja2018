#include "display.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>

#include "util.h"

namespace {
// PINS
const unsigned int LE = 13;   // Pull up to signal end of serial input
const unsigned int OE = 6;    // Pull down/up to turn display on/off

const int MAX_RPM = 3800;     // Max RPM expected (governor)

// CHANNEL MAP
// LSB
// [0, 11]  - 12 rpm led's
// 12       - warning temp
// 13       - warning battery
// 14       - info brake
// 15       - info data logging
// [16, 22] - 7 segment display for 10's digit
// [23, 29] - 7 segment display for 1's digit
// [30, 31] - UNUSED
// MSB

// Shift to align things to channel map
const unsigned int SHIFT_STATUS_LEDS = 12;  // Shift for status leds
const unsigned int SHIFT_7SEG10 = 16;       // Shift for 10's digit
const unsigned int SHIFT_7SEG1  = 23;       // Shift for 1's digit

// Maps segment labels to bit values
// Labels are as follows:
//  AA
// F  B
// F  B
//  GG
// E  C
// E  C
//  DD
const unsigned int A = 1 << 0;
const unsigned int B = 1 << 1;
const unsigned int C = 1 << 2;
const unsigned int D = 1 << 3;
const unsigned int E = 1 << 4;
const unsigned int F = 1 << 5;
const unsigned int G = 1 << 6;

// Converts digit to segment representation
const unsigned int dig2seg[] = {
  A | B | C | D | E | F,      // 0
  B | C,                      // 1
  A | B | G | E | D,          // 2
  A | B | G | C | D,          // 3
  F | G | B | C,              // 4
  A | F | G | C | D,          // 5
  A | F | E | D | C | G,      // 6
  A | B | C,                  // 7
  A | B | G | E | D | C | F,  // 8
  G | F | A | B | C,          // 9
};

// SPI Constants
const unsigned BAUD_RATE = 10000000;  // 10MHz
// 32 bits per word, and using auxillary device (SPI1)
const unsigned SPI_FLAGS = (32 << 16) | (1 << 8);

// pigpiod handles
int pi = -1;
int spi = -1;
}  // anonynous namespace

namespace display {

void init() {
  // Only make a connection if one does not already exist
  if (pi <= 0) {
    pi = assert_success(pigpio_start(nullptr, nullptr));
  }
}

void begin() {
  assert_success(set_mode(pi, LE, PI_OUTPUT));
  assert_success(set_mode(pi, OE, PI_OUTPUT));
  // Open spi on channel 0 (CS0 for bus 1)
  if (spi < 0)
    spi = assert_success(spi_open(pi, 0, BAUD_RATE, SPI_FLAGS));

  // Turn display off by default
  assert_success(gpio_write(pi, OE, PI_HIGH));
}

void update(unsigned int rpm, unsigned int mph, unsigned int status_flags) {
  print_assert("Warning Flags out of acceptable range",
        status_flags < STATUS_UNKNOWN);

  // BUILD BIT REPRESENTATION OF CHANNELS
  uint32_t channels;

  // LEDs
  // TODO: Verify that taking a floor is the desired effect
  int num_leds = (int) util::clamp((float) 0, (float) 12, (float) 0,
      (float) MAX_RPM, (float) rpm);
  // 0xFFF is 12 bits, and we want to shift each led not in use out.
  if (num_leds > 0)
    channels = 0xFFF >> (12 - num_leds);
  else
    channels = 0;

  // Warnings
  channels |= status_flags << SHIFT_STATUS_LEDS;

  // 7 Segment Displays
  // If in excess of 99 mph, 100's place is lost
  auto divmod = div(mph % 100, 10);
  if (divmod.quot)  // Leave 10's digit 0'd out when single digit number
    channels |= dig2seg[divmod.quot] << SHIFT_7SEG10;
  channels |= dig2seg[divmod.rem] << SHIFT_7SEG1;

  // UPDATE DISPLAY
  // Turn off display before update
  assert_success(gpio_write(pi, OE, PI_HIGH));

  // DEBUG
  // fprintf(stdout, "Channels: 0x%08X\n", channels);
  assert_success(spi_write(pi, spi, (char *) &channels, 4));

  assert_success(gpio_write(pi, LE, PI_HIGH));  // Signal end of comm
  assert_success(gpio_write(pi, LE, PI_LOW));

  assert_success(gpio_write(pi, OE, PI_LOW));  // Turn display back on
}

void end() {
  assert_success(gpio_write(pi, OE, PI_HIGH));
  if (spi >= 0) {
    assert_success(spi_close(pi, spi));
    spi = -1;
  }
}

void close() {
  // Close connection only if there is one
  if (pi >= 0) {
    pigpio_stop(pi);
    pi = -1;
  }
}

}  // namespace display
