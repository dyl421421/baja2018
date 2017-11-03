#include "adc.h"

#include <cstdio>
#include <cassert>

#include <pigpiod_if2.h>

#include "util.h"

namespace adc {
namespace {
const unsigned BAUD_RATE = 3600000;  // Will round down to next avail rate
const unsigned SPI_FLAGS = 0;  // All default options

int pi = -1;  // Returned from pigpio_start, think of as fd for pigpio conns
int spi0 = -1, spi1 = -1;  // Similar to pi, but for spi channels
}  // anonymous namespace

void init() {
  // Only connect if no connection already exists
  if (pi < 0) {
    pi = assert_success(pigpio_start(nullptr, nullptr));
  }
}

void begin() {
  print_assert("Attempted to read spi without an open connection, "
        "make sure to call init!", pi >= 0);

  // Only connect if not connected
  if (spi0 < 0) {
    spi0 = assert_success(spi_open(pi, 0, BAUD_RATE, SPI_FLAGS));
  }

  // Only connect if not connected
  if (spi1 < 0) {
    spi1 = assert_success(spi_open(pi, 1, BAUD_RATE, SPI_FLAGS));
  }
}

void end() {
  if (spi0 >= 0) {
    assert_success(spi_close(pi, spi0));
    spi0 = -1;
  }

  if (spi1 >= 0) {
    assert_success(spi_close(pi, spi1));
    spi1 = -1;
  }
}

void close() {
  if (pi >= 0) {
    pigpio_stop(pi);
    pi = -1;
  }
}

int get(ChipSelect cs, char ch) {
  print_assert("Attempted to get ADC value without connections to pigpio "
      "daemon", pi >= 0);
  print_assert("ch outside of acceptable range", ch >= 0 && ch <= 7);
  unsigned spi_handle = (cs == CS0 ? spi0 : spi1);

  char buf[3];
  buf[0] = 0x01;  // Start bit, right aligned in first byte
  
  buf[1] = 0x80;  // Signifies single mode (0x00 would be diff mode)
  buf[1] |= (ch << 4);  // Shift chip number next to mode bit
  
  // DEBUG
  // printf("\nBefore: %02X.%02X.%02X\n", buf[0], buf[1], buf[2]);

  // Rest of buf will store answer
  int count = spi_xfer(pi, spi_handle, buf, buf, 3);
  print_assert("SPI tranfser failed", count == 3);

  // DEBUG
  // printf("After: %02X.%02X.%02X\n", buf[0], buf[1], buf[2]);

  print_assert("Null bit not set by adc!", (buf[1] & 0x4) == 0);

  int result = buf[1] << 8;  // 2 LSB of buf[1] are the 2 MSB of result
  result |= buf[2];  // buf[2] are 8 LSB of result (total 10 bit)

  return result & 0x3FF;  // truncate to last 10 bits
}
}  // namespace adc
