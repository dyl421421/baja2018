#ifndef UTIL_H_
#define UTIL_H_

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <pigpiod_if2.h>

#define IF_ERR_PRINT_PIGPIO_ERR(errnum) {\
    if (errnum < 0) {\
      fprintf(stderr, "\n[%s:%d] PiGPIO Daemon Error: %s\n",\
          __FILE__, __LINE__, pigpio_error((errnum)));\
    }\
  }

// The following creates an inline lambda for scoping and multiple statements,
// and then evaluaes to the value of expr.
// With -03, the lambda call WILL be inlined!
// NOTE: if anything needs to be captured (for expr), it will be captured as
//       a reference.
#define assert_success(expr) ([&]() {\
    int ret = (expr);\
    IF_ERR_PRINT_PIGPIO_ERR(ret);\
    assert((#expr, ret >= 0));\
    return ret;\
  }())

// The following prints an error in production, and aborts in debug
#ifdef NDEBUG
#define print_assert(errstr, expr) \
  if (!(expr))\
    fprintf(stderr, "\n[%s:%d] Error: %s\n",\
        __FILE__, __LINE__, (errstr));
#else
#define print_assert(errstr, expr) assert(((errstr), (expr)))
#endif  // NDEBUG

namespace util {
// Clamsps a value on range [min, max] to range [new_min, new_max] through
// linear scaling
inline double clamp(double new_min, double new_max, double min, double max,
    double value) {
  return new_min + (value - min) * (new_max - new_min) / (max - min);
}
inline float clamp(float new_min, float new_max, float min, float max,
    float value) {
  return new_min + (value - min) * (new_max - new_min) / (max - min);
}

// Preforms MSB CRC-8 with polynomial 0x07
inline uint8_t crc8(uint64_t data) {
  for (int i = 0; i < 64; ++i) {  // data can be up to 64 bits
    bool divisible = data & (1ULL << 63);  // Checks if 1 in msb
    data <<= 1;                            // Shifts out top digit regardless
    if (divisible)                         // "Subtracts" if divisible
      data ^= (0x07ULL << 56);
  }
  
  // Remainder is MSB of data
  return data >> 56;
}
}  // namespace util

#endif  // UTIL_H_
