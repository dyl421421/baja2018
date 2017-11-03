#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <memory>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <vector>

#include <pigpiod_if2.h>

#include "csv.h"
#include "display.h"
#include "sensors.h"

using namespace std;
using namespace sensors;

const float kCvtWarnTempF = 200;
const float kLowBatteryVoltage = 11.1;
const unsigned kMaxFileNum = 9999;
const char *kFilenameFormat = "/home/pi/DAQ/RECORD_%04d.csv";
// Path and prefix + number + extension + null byte.
const unsigned kFilenameLen = 20 + 4 + 4 + 1;
const char *kCsvHeaders[] = {
  "Time (s)",
  "Accelerometer X",
  "Accelerometer Y",
  "Acceleroemter Z",
  "Ambient Temp",
  // Sensors needed for Display.
  "CVT Temp",
  "Rear HAL",
  "Tachometer",
  "Battery Voltage",
  // Testing Only from this point forward [9-21).
  "Front Right HAL",
  "Front Left HAL",
  "Front Breakline Pressure",
  "Rear Breakline Pressure",
  "Steering Angle",
  "Front Right Suspension Travel",
  "Front Left Suspension Travel",
  "Rear Right Suspension Travel",
  "Rear Left Suspension Travel",
  "Front Right Rotor Temp",
  "Front Left Rotor Temp",
  "Rear Rotor Temp",
};
const unsigned kTestingStartPos = 9;
const unsigned kHeadersLen = 21;

// When set to true, display updates don't happen, and display stays locked.
atomic<bool> display_locked(false);
// Thread pointer for thread that unlocks display after a timeout.
pthread_t *display_unlock_thread = nullptr;

// Displays a file number instead of mph for timeout_seconds
void display_file_num(unsigned file_num, double timeout_seconds) {
  // Lock display, and update with file num.
  display_locked = true;
  display::update(0, file_num, display::INFO_DATA_LOGGING);

  stop_thread(display_unlock_thread);  // Stops thread if already active.

  // Allocate storage for timeout on heap
  double *timeout = new double;
  *timeout = timeout_seconds;
  
  // Passes timeout into the thread, which takes ownership of the pointer.
  display_unlock_thread = start_thread([](void *data) -> void * {
        unique_ptr<double> timeout((double *) data);  // Takes ownership.

        time_sleep(*timeout);    // Wait for timeout.
        display_locked = false;  // Unlock display.

        return nullptr;
      }, (void *) timeout);
}

// Holds Csv object.
unique_ptr<Csv> csv;

// Opens a CSV, optionally in testing mode, with next avail file number.
// Returns the file number used (if greater than kMaxFileNum,
//  then no file was opened).
unsigned OpenCsv(bool testing) {
  struct stat buffer;
  char filename[kFilenameLen];
  unsigned file_num;

  // Try generating filenames until the file is available.
  for (file_num = 0; file_num <= kMaxFileNum; ++file_num) {
    snprintf(filename, kFilenameLen, kFilenameFormat, file_num);

    // If an error occurs while stat'ing the file, then it doesn't exist.
    if (stat(filename, &buffer) == -1)
      break;
  }

  // If no file is available, don't open a file.
  if (file_num > kMaxFileNum)
    return file_num;

  csv.reset(new Csv(filename,
        testing ?
        // If testing, use all headers.
        vector<const char *>(kCsvHeaders, kCsvHeaders + kHeadersLen) :
        // Otherwise, use only the headers up until the testing headers.
        vector<const char *>(kCsvHeaders, kCsvHeaders + kTestingStartPos)));

  display_file_num(file_num, 1.5);

  return file_num;
}

// Closes the csv if its open.
void CloseCsv(unsigned file_num) {
  if (csv) {
    csv.reset();
    display_file_num(file_num, 1.5);
  }
}

// Testing is true if the last opened csv was in testing mode.
// File_num is the file_num of the last opened csv.
// Tv_start is the time the csv was last opened.
void loop(bool &testing, int &file_num, struct timeval &tv_start) {
  // If daq switch is on, and csv is not open, open it.
  if (is_daq() && !csv) {
    // If not nan, then testing sensors are attached (store state in testing).
    file_num = OpenCsv((testing = !isnan(front_right_rotor_temp())));
    gettimeofday(&tv_start, nullptr);
  } else if (!is_daq() && csv) {
    CloseCsv(file_num);
  }

  // If logging, these values always get logged.
  if (csv) {
    struct timeval tv_now;
    gettimeofday(&tv_now, nullptr);
    uint64_t time_usec = (tv_now.tv_sec - tv_start.tv_sec) * 1000000 +
                          tv_now.tv_usec - tv_start.tv_usec;

    auto acc_xyz = accelXYZ();
    (*csv) << time_usec << get<0>(acc_xyz) << get<1>(acc_xyz)
      << get<2>(acc_xyz) << amb_temp();
  }

  // Readings for display (also needed for logging).
  float cvt = cvt_temp();
  float mph = rear_hal();
  float rpm = rpm_tach();
  float bat_voltage = battery_voltage();

  // Only update display when not locked.
  if (!display_locked) {
    unsigned status = display::STATUS_NONE;
    if (cvt >= kCvtWarnTempF)
      status |= display::WARNING_TEMP;
    if (bat_voltage <= kLowBatteryVoltage)
      status |= display::WARNING_BATTERY;
    if (is_brake())
      status |= display::INFO_BRAKE;
    if (csv)
      status |= display::INFO_DATA_LOGGING;

    display::update(rpm, mph, status);
  }

  // Log display values, and sometimes more
  if (csv) {
    (*csv) << cvt << mph << rpm << bat_voltage;
    if (testing) {
      (*csv) << front_right_hal() << front_left_hal()
        << front_brake_line_pressure() << rear_brake_line_pressure()
        << steering_angle()
        << front_right_suspension() << front_left_suspension()
        << rear_right_suspension() << rear_left_suspension()
        << front_right_rotor_temp() << front_left_rotor_temp()
        << rear_rotor_temp();
    }
    (*csv) << Csv::LINE_BREAK;
  }
}

// States for shutdown procedures.
atomic<bool> run(true);             // While true, main loop runs.
atomic<bool> done(false);           // Set to true when main loop exits.
atomic<bool> wait_on_done(false);   // True if main loop should wait on exit.

// Logic for shutting down the pi. Replaces this process with sudo executing
// shutdown.
const char *shutdown_args[] = {"/usr/bin/sudo", "shutdown", "now", nullptr};
void shutdown() {
  // Indiciate to main thread to wait when finished, instead of exiting.
  // Allows shutdown command to execute before process exits.
  wait_on_done = true;

  // Stop the main loop, and wait for it to finish.
  run = false;
  while (!done)
    this_thread::yield();

  // NOTE: This const cast is generally considered acceptable seeing as
  //       execv predates const and doesn't actually edit it's arguments.
  // This call replaces the current process with the shell issuing this command.
  execv(shutdown_args[0], const_cast<char *const *>(shutdown_args));

  // If execv returns, there was a failure.
  cerr << "Failed to shutdown Pi" << endl;

  // On failure, allow program to exit properly.
  wait_on_done = false;
}

int main(int argc, char **argv) {
  // Act as log headers
  cout << "Driver Started" << endl;
  cerr << "Driver Started" << endl;

  const double kPeriodSeconds = 0;

  // Catch SIGINT, and just exit the mainloop and close cleanly.
  struct sigaction sig_int_handler;
  sig_int_handler.sa_handler = [](int) { run = false; };  // Exit main loop.
  sigemptyset(&sig_int_handler.sa_mask);
  sig_int_handler.sa_flags = 0;

  sigaction(SIGINT, &sig_int_handler, nullptr);

  display::init();
  sensors::init();
  display::begin();
  sensors::begin();

  sensors::on_shutdown(&shutdown);

  // For passing to loop by reference (can be edited).
  bool testing;
  int file_num;
  struct timeval tv_start;

  while (run) {
    double loop_start = time_time();
    loop(testing, file_num, tv_start);
    double loop_elapsed = time_time() - loop_start;

    if (loop_elapsed < kPeriodSeconds)
      time_sleep(kPeriodSeconds - loop_elapsed);
  }

  CloseCsv(file_num);  // Close if open.

  display::end();
  sensors::end();
  display::close();
  sensors::close();

  done = true;

  while (wait_on_done)
    this_thread::yield();

  return 0;
}
