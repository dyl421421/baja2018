#include "ir_temp.h"

#include <cassert>
#include <cstdint>

#include <pigpiod_if2.h>

#include "util.h"

namespace ir_temp {
namespace {
int pi = -1;                    // Handle for connecting to pigpio daemon
int i2c[] = {-1, -1, -1, -1};   // Handles for connecting to i2c devices

// Connection Constants
const uint8_t kBus = 1;
const uint8_t kAddrs[] = {0x5A, 0x5B, 0x5C, 0x5D};

// MLX90614 Constants
const uint8_t kRamAddrTA    = 0x06;
const uint8_t kRamAddrTObj1 = 0x07;

// Internal get temperature in Fahrenheit
template <const uint8_t RAM_ADDR>
Reading get_temp_F(Device d) {
  print_assert("Attempting to read from a device without a valid connection to "
      "the pigpio daemon", pi >= 0);

  // Return val
  Reading result;

  if (i2c[d] < 0) {
    result.stat = BAD_HANDLE;
    return result;
  }

  // I2C Command String
  // This allows commands to be sequences w/o end bits, so repeated start!
  char i2c_cmd[] = {
    0x2,                  // Switch Combined Flag On
    0x7, 0x1, RAM_ADDR,   // Write 1 byte (RAM_ADDR)
    0x6, 0x3,             // Read 3 bytes (DATA_LOW, DATA_HIGH, PEC)
    0x0,                  // End cmd sequence
  };
  char i2c_buf[3];

  int count = assert_success(i2c_zip(pi, i2c[d],
        i2c_cmd, sizeof(i2c_cmd),
        i2c_buf, sizeof(i2c_buf)));
  print_assert("Wrong number of bytes recieved", count == sizeof(i2c_buf));
  if (count != sizeof(i2c_buf)) {
    result.stat = BAD_RETURN_LEN;
    return result;
  }

  uint16_t word = i2c_buf[0] | (i2c_buf[1] << 8);

  // Entire transaction excluding S, Sr, A, Na, P and PEC
  // Needs to be constructed bc PEC calculation includes all of these bits
  uint64_t data = (uint64_t) ((kAddrs[d] << 1) | 0)  << 32 |  // SA_Wr
                  (uint64_t) RAM_ADDR                << 24 |  // Command
                  (uint64_t) ((kAddrs[d] << 1) | 1)  << 16 |  // SA_R
                  (uint64_t) i2c_buf[0]              <<  8 |  // LSB
                  (uint64_t) i2c_buf[1];                      // MSB
  uint8_t crc8 = util::crc8(data);

  uint8_t pec = i2c_buf[2];  // Packet Error Code
  print_assert("Packet Error Code does not match CRC-8 (0x07 MSB) polynomial "
        "remainder", pec == crc8);
  if (pec != crc8) {
    result.stat = CRC8_MISMATCH;
    return result;
  }

  // DEBUG
  // fprintf(stderr, "Values: 0x%02x 0x%02x\n", pec, crc8);

  // word * resolution is temp in K
  result.val = (word * 0.02 - 273.15) * 9 / 5 + 32;
  return result;
}
}  // anonymous namespace

void init() {
  // Only connect if no connection already exists
  if (pi < 0) {
    pi = assert_success(pigpio_start(nullptr, nullptr));
  }
}

void begin() {
  print_assert("Attempting to connect to i2c without a valid connection to the "
      "pigpio daemon, must call init() before begin()", pi >= 0);

  for (int i = 0; i < NUM_DEVICES; ++i) {
    // Only connect if a valid connection does not already exist
    if (i2c[i] < 0) i2c[i] = assert_success(i2c_open(pi, kBus, kAddrs[i], 0));
  }
}

void end() {
  // Looping by reference to allow reassign to -1 when needed
  for (int &handle : i2c) {
    if (handle >= 0) {
      assert_success(i2c_close(pi, handle));
      handle = -1;
    }
  }
}

void close() {
  if (pi >= 0) {
    pigpio_stop(pi);
    pi = -1;
  }
}

Reading get_obj(Device d) {
  return get_temp_F<kRamAddrTObj1>(d);
}

Reading get_amb(Device d) {
  return get_temp_F<kRamAddrTA>(d);
}
}  // namespace ir_temp
