#include "sensors.h"

#include <iostream>

#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <mutex>

#include <pigpiod_if2.h>

#include "util.h"

using namespace std;

namespace sensors {
namespace {
// GPIO constants
const int kShutdownTogglePin = 23;
const int kDaqSwitchPin = 24;
const int kBrakePin = 18;

// TODO: Check that this is correct.
// Daq active level (high or low).
const unsigned kDaqActiveState = PI_HIGH;

// Active level for Brake light line.
const unsigned kBrakeLightActiveState = PI_HIGH;

// Shutdown timeout. Takes a press of this long to shutdown pi.
const unsigned kShutdownTimeoutMs = 250;  // In milliseconds.

// Bounce Time
const int kBounceTime = 20000;  // 20ms [us]

// State for managing connection for GPIO access.
int pi = -1;
int daq_cb = -1, shutdown_cb = -1, brake_cb = -1;
pthread_t *user_shutdown_cb_thread = nullptr;
atomic<bool> daq_state(false);
atomic<bool> brake_state(false);

// State for user shutdown callback
function<void(void)> user_shutdown_cb;
mutex user_shutdown_cb_mutex;  // GUARDS user_shutdown_cb

// Called on every edge, level indicates state.
void daq_switched(int, unsigned, unsigned level, uint32_t) {
  daq_state = (level == kDaqActiveState);
}

void brake_switched(int, unsigned, unsigned level, uint32_t) {
  brake_state = (level == kBrakeLightActiveState);
}

// Called on every edge, level indicates state. PI_TIMEOUT means watchdog fired.
void shutdown_state_changed(int, unsigned, unsigned level, uint32_t) {
  if (level == PI_LOW) {  // Button depressed (set watchdog to wait out press).
    assert_success(set_watchdog(pi, kShutdownTogglePin, kShutdownTimeoutMs));
    return;
  } 
  
  // Remove watchdog on all other triggers.
  assert_success(set_watchdog(pi, kShutdownTogglePin, 0));

  if (level == PI_TIMEOUT) { // Watchdog timed out.
    lock_guard<mutex> user_shutdown_cb_mutex_lock(user_shutdown_cb_mutex);
    
    // Run callback in new child thread (this callback shouldn't hang on it).
    if (user_shutdown_cb) {
      stop_thread(user_shutdown_cb_thread);  // Stop previous thread if exists.
      user_shutdown_cb_thread = start_thread([](void *) -> void * {
            user_shutdown_cb();
            return nullptr;
          }, nullptr);
    }
  }
}

// Enum mapping names to chip and channel values. 4th-bit is chip.
enum Adc : char {
  // Front Dongle
  FL_HAL    = 0x0,  // CE0, C0
  FR_HAL    = 0x1,  // CE0, C1
  F_BRAKE   = 0x2,  // CE0, C2
  R_BRAKE   = 0x3,  // CE0, C3
  STEERING  = 0x4,  // CE0, C4
  FL_SUS    = 0x5,  // CE0, C5
  FR_SUS    = 0x6,  // CE0, C6
  // N.C.              CE0, C7
  // Rear Dongle
  R_HAL     = 0x8,  // CE1, C0
  RPM_TACH  = 0x9,  // CE1, C1
  RL_SUS    = 0xA,  // CE1, C2
  RR_SUS    = 0xB,  // CE1, C3
  BATTERY   = 0xC,  // CE1, C4
  // N.C.              CE2, C5-7
};

const float kPi = atan(1) * 4;

// Internal wrapper to adc::get
int adc_get(Adc sensor) {
  return adc::get((adc::ChipSelect) ((sensor & 0x8) >> 3), sensor & 0x7);
}

// Gets the suspension travel in the length units of a and b. Phi must be in
// radians.
//
// a is one of the support sides of the suspension.
// b is one of the support sides of the suspension.
// phi is the angle offset of the 0 position of the potentiometer from the
//     nearest support side.
float sus_travel(Adc sus_pot, float a, float b, float phi) {
  errno = 0;  // For sqrt domain check

  float theta = util::clamp((float) 0, (float) kPi/3, (float) 0, (float) 1023,
      adc_get(sus_pot));
  float travel = sqrt(a*a + b*b - 2*a*b*cos(theta + phi));
  
  if (errno == EDOM)
    return NAN;

  return travel;
}

// Gets a hal reading and converts it to mph based on the min_hz and max_hz
// the specified freq2voltage converter can read, and the drive train ratio.
//
// min_hz the minimum frequency reading of the freq2voltage converter
// max_hz the maximum frequency reading of the freq2voltage converter
// dt_ratio the drive train ratio (mph/hz).
float hal_to_mph(Adc hal, int min_hz, int max_hz, float dt_ratio) {
  float freq = util::clamp((float) min_hz, (float) max_hz, (float) 0,
      (float) 1023, adc_get(hal));
  return freq * dt_ratio;
}
}  // anonymous namespace

// ADC

float front_left_hal() {
  const float kMinHz = 0;
  const float kMaxHz = 5000;
  const float kDriveTrainRatio = 1/6.81;

  return hal_to_mph(FL_HAL, kMinHz, kMaxHz, kDriveTrainRatio);
}

float front_right_hal() {
  const float kMinHz = 0;
  const float kMaxHz = 5000;
  const float kDriveTrainRatio = 1/6.81;

  return hal_to_mph(FR_HAL, kMinHz, kMaxHz, kDriveTrainRatio);
}

// Gets pressure in psi
float front_brake_line_pressure() {
  const float kMinReadout = 0.5 * 1023;
  const float kMaxReadout = 4.5 * 1023;

  return util::clamp(0, 2000, kMinReadout, kMaxReadout, adc_get(F_BRAKE));
}

float rear_brake_line_pressure() {
  const float kMinReadout = 0.5 * 1023;
  const float kMaxReadout = 4.5 * 1023;

  return util::clamp(0, 2000, kMinReadout, kMaxReadout, adc_get(R_BRAKE));
}

float steering_angle() {
  const float kMinWheelAngle = -90;
  const float kMaxWheelAngle = 90;
  const float kMinPotReadout = 0;
  const float kMaxPotReadout = 1023;

  return util::clamp(kMinWheelAngle, kMaxWheelAngle,
      kMinPotReadout, kMaxPotReadout, adc_get(STEERING));
}

float front_left_suspension() {
  const float kSideA = 10;
  const float kSideB = 7;
  const float kPhi = (float) 2/3 * kPi;

  return sus_travel(FL_SUS, kSideA, kSideB, kPhi);
}

float front_right_suspension() {
  const float kSideA = 10;
  const float kSideB = 7;
  const float kPhi = (float) 2/3 * kPi;

  return sus_travel(FR_SUS, kSideA, kSideB, kPhi);
}

float rear_hal() {
  const float kMinHz = 0;
  const float kMaxHz = 5000;
  const float kDriveTrainRatio = 1/6.81;

  return hal_to_mph(R_HAL, kMinHz, kMaxHz, kDriveTrainRatio);
}

// Min and Max based on freq2voltage reading range, not expected output.
float rpm_tach() {
  const float kMinHz = 0;
  const float kMaxHz = 3800;

  return util::clamp(kMinHz, kMaxHz, 0, 1023, adc_get(RPM_TACH));
}

float rear_left_suspension() {
  const float kSideA = 10;
  const float kSideB = 7;
  const float kPhi = (float) 2/3 * kPi;

  return sus_travel(RL_SUS, kSideA, kSideB, kPhi);
}

float rear_right_suspension() {
  const float kSideA = 10;
  const float kSideB = 7;
  const float kPhi = (float) 2/3 * kPi;

  return sus_travel(RR_SUS, kSideA, kSideB, kPhi);
}

float battery_voltage() {
  const float kChargedVoltage = 12.5;
  const float kMaxReadout = 976;

  return util::clamp(0, kChargedVoltage, 0, kMaxReadout, adc_get(BATTERY));
}

// ACCEL

tuple<float, float, float> accelXYZ() {
  auto reading = accel::get_acceleration();
  if (reading.stat != accel::OK)
    return make_tuple(NAN, NAN, NAN);

  return make_tuple((float) reading.x, (float) reading.y, (float) reading.z);
}

// TEMP

float amb_temp() {
  // Arbitrarilty read ambient temp from CVT
  auto reading = ir_temp::get_amb(ir_temp::CVT_BELT);
  if (reading.stat != ir_temp::OK)
    return NAN;

  return reading.val;
}

float cvt_temp() {
  auto reading = ir_temp::get_obj(ir_temp::CVT_BELT);
  if (reading.stat != ir_temp::OK)
    return NAN;

  return reading.val;
}

float rear_rotor_temp() {
  auto reading = ir_temp::get_obj(ir_temp::R_ROTOR);
  if (reading.stat != ir_temp::OK)
    return NAN;

  return reading.val;
}

float front_left_rotor_temp() {
  auto reading = ir_temp::get_obj(ir_temp::FL_ROTOR);
  if (reading.stat != ir_temp::OK)
    return NAN;

  return reading.val;
}

float front_right_rotor_temp() {
  auto reading = ir_temp::get_obj(ir_temp::FR_ROTOR);
  if (reading.stat != ir_temp::OK)
    return NAN;

  return reading.val;
}

bool is_daq() {
  return daq_state;
}

bool is_brake() {
  return gpio_read(pi, kBrakePin) == kBrakeLightActiveState;
}

void on_shutdown(const function<void(void)> &callback) {
  lock_guard<mutex> user_shutdown_cb_mutex_lock(user_shutdown_cb_mutex);

  user_shutdown_cb = callback;
}

void init() {
  accel::init();
  adc::init();
  ir_temp::init();

  if (pi < 0)
    pi = assert_success(pigpio_start(nullptr, nullptr));
}

void begin() {
  accel::begin();
  adc::begin();
  ir_temp::begin();

  // Setup GPIO pins
  assert_success(set_mode(pi, kShutdownTogglePin, PI_INPUT));
  assert_success(set_mode(pi, kDaqSwitchPin, PI_INPUT));
  assert_success(set_mode(pi, kBrakePin, PI_INPUT));

  assert_success(set_pull_up_down(pi, kShutdownTogglePin, PI_PUD_UP));
  assert_success(set_pull_up_down(pi, kDaqSwitchPin, PI_PUD_UP));
  assert_success(set_pull_up_down(pi, kBrakePin, PI_PUD_DOWN));

  if (shutdown_cb < 0)
    shutdown_cb = assert_success(callback(pi, kShutdownTogglePin, EITHER_EDGE,
          &shutdown_state_changed));
  if (daq_cb < 0)
    daq_cb = assert_success(callback(pi, kDaqSwitchPin, EITHER_EDGE,
          &daq_switched));
  if (brake_cb < 0)
    brake_cb = assert_success(callback(pi, kBrakePin, EITHER_EDGE,
          &brake_switched));

  assert_success(set_glitch_filter(pi, kShutdownTogglePin, kBounceTime));
  assert_success(set_glitch_filter(pi, kDaqSwitchPin, kBounceTime));
  assert_success(set_glitch_filter(pi, kBrakePin, kBounceTime));
}

void end() {
  accel::end();
  adc::end();
  ir_temp::end();

  assert_success(set_pull_up_down(pi, kShutdownTogglePin, PI_PUD_OFF));
  assert_success(set_pull_up_down(pi, kDaqSwitchPin, PI_PUD_OFF));
  assert_success(set_pull_up_down(pi, kBrakePin, PI_PUD_OFF));

  if (shutdown_cb >= 0)
    assert_success(callback_cancel(shutdown_cb));
  if (daq_cb >= 0)
    assert_success(callback_cancel(daq_cb));
  if (brake_cb >= 0)
    assert_success(callback_cancel(brake_cb));

  assert_success(set_glitch_filter(pi, kShutdownTogglePin, 0));
  assert_success(set_glitch_filter(pi, kDaqSwitchPin, 0));
  assert_success(set_glitch_filter(pi, kBrakePin, 0));
}


void close() {
  accel::close();
  adc::close();
  ir_temp::close();

  if (pi >= 0) {
    pigpio_stop(pi);
    pi = -1;
  }
}
}  // namespace sensors
