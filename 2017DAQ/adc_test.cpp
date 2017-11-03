#include <iostream>
#include <thread>
#include <unistd.h>

#include "adc.h"

using namespace std;

const int kSleepTimeMicros = 250000;

void adc_loop() {
  while (true) {
    cout << "Chip 1, Channel " << 4 << ": " << adc::get(adc::CS1, 4) << endl;
    usleep(kSleepTimeMicros);
  }
}

int main(int argc, char **argv) {
  adc::init();
  adc::begin();

  cout << "Press <Enter> to terminate" << endl;

  // Scope for thread
  {
    thread adc_thread(&adc_loop);

    while (cin.get() != '\n');  // Wait for enter
  }
  
  adc::end();
  adc::close();
  return 0;
}
