#include "accel.h"

#include <pigpiod_if2.h>

#include "util.h"

namespace accel {
namespace {
// Connection Handles
int pi = -1;
int i2c = -1;

// Connection Constants
const uint8_t kBus = 1;
const uint8_t kAddr = 0x18;  // NOTE: can be changed to 0x19 if needed

// LIS3DH Read Registers
const uint8_t kRamAddrAdcs = 0x08;
const uint8_t kRamAddrTemp = 0x0C;
const uint8_t kRamAddrAxes = 0x28;

// LIS3DH Config Registers
// CTRL_REG_1:
//  MSB |ODR3|ODR2|ODR1|ODR0|LPen|Zen|Yen|Xen|
//  ODR: Data rate
//  LPen: Low pwr mode
//  *en: Axes enable
const uint8_t kRamAddrCtrlReg1 = 0x20;
// CTRL_REG_4:
//  MSB |BDU|BLE|FS1|FS0|HR|ST1|ST0|SIM| LSB
//  BDU: Block Update until read
//  FS*: Used to set device scale (scale is +/- [1 << ((FS1 << 1)|FS0 + 1)])
//  HR: High Resolution Mode
//  ST*: Self Test Modes (MUST BE OFF IN PROD)
const uint8_t kRamAddrCtrlReg4 = 0x23;
// TEMP_CFG_REG
//  MSB |ADC_PD|TEMP_EN|...Unused...| LSB
//  ADC_PD: ADC Enable
//  TEMP_EN: Temperature Enable (NOTE: Runs on 3rd ADC Channel)
const uint8_t kRamAddrTempCfgReg = 0x1F;

// Internal I2C Repeated Start read 16-bit words access
Status i2c_repeated_read_words(uint16_t *buf, uint8_t num_words,
    uint8_t start_addr) {
  print_assert("Attempting to read from device without a connection to the "
      "pigpio daemon", pi >= 0);
  print_assert("Attempting to read from device without an I2C connection",
      i2c >= 0);

  char i2c_cmd[] = {
    0x3,                            // Turn repeated start on
    0x7, 0x1,                       // Write 1 byte
    (char) (0x80 | start_addr),     // MSB inidicates multiple read
    0x6, (char) (num_words << 1),   // Read num_words * 2 bytes
    0x0                             // End comm
  };

  int count = assert_success(i2c_zip(pi, i2c, i2c_cmd, sizeof(i2c_cmd),
        (char *) buf, num_words << 1));
  print_assert("Wrong number of bytes recieved", count == num_words << 1);
  if (count != (num_words << 1)) return BAD_RETURN_LENGTH;

  return OK;
}

// Internal I2C Repeated Start read byte access
Status i2c_repeated_read_byte(uint8_t *byte, uint8_t addr) {
  print_assert("Attempting to read from device without a connection to the "
      "pigpio daemon", pi >= 0);
  print_assert("Attempting to read from device without an I2C connection",
      i2c >= 0);

  char i2c_cmd[] = {
    0x2,              // Turn repeated start on
    0x7, 0x1, addr,   // Write one byte (ram addr)
    0x6, 0x1,         // Read one byte
    0x0               // End comm
  };

  int count = assert_success(i2c_zip(pi, i2c, i2c_cmd, sizeof(i2c_cmd),
        (char *) byte, 1));
  print_assert("Wrong number of bytes recieved", count == 1);
  if (count != 1) return BAD_RETURN_LENGTH;

  return OK;
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
  
  if (i2c < 0)
    i2c = assert_success(i2c_open(pi, kBus, kAddr, 0));

  // Accelerometer Configuration
  // Set speed to 50Hz and enable all axes
  assert_success(i2c_write_byte_data(pi, i2c, kRamAddrCtrlReg1, 0x47));
  // Enable Block Update, High Res, and set range to +/- 4
  assert_success(i2c_write_byte_data(pi, i2c, kRamAddrCtrlReg4, 0x98));
  // Enable Temp and ADC
  assert_success(i2c_write_byte_data(pi, i2c, kRamAddrTempCfgReg, 0xC0));
}

void end() {
  if (i2c >= 0) {
    i2c_close(pi, i2c);
    i2c = -1;
  }
}

void close() {
  if (pi >= 0) {
    pigpio_stop(pi);
    pi = -1;
  }
}

AccReading get_acceleration() {
  const double divider = (1 << 15) / 4;  // 11 is bc signed 16-bit, 4 bc scale
  AccReading res;
  int16_t buf[3];

  res.stat = i2c_repeated_read_words((uint16_t *) buf, 3, kRamAddrAxes);
  // +/- 4 g's so div by 4
  res.x = (double) buf[0] / divider;
  res.y = (double) buf[1] / divider;
  res.z = (double) buf[2] / divider;  // 8190

  return res;
}

AdcReading get_adc() {
  AdcReading res;
  uint16_t buf[3];

  // TODO: x and y are labeled inversed on chip, ask him about that
  res.stat = i2c_repeated_read_words((uint16_t *) buf, 3, kRamAddrAdcs);
  res.out1 = buf[0];
  res.out2 = buf[1];
  res.out3 = buf[2];

  return res;
}

TempReading get_temperature() {
  TempReading res;
  uint16_t raw;
  
  // TODO: Figure out temperature
  res.stat = i2c_repeated_read_words(&raw, 1, kRamAddrTemp);
  res.temp = raw - 1575;

  return res;
}
}  // namespace accel
