#include "accel.h"

#include <cstdio>
#include <pigpiod_if2.h>

using namespace std;

int main(int argc, char **argv) {
  printf("Accelerometer Output (Press <Enter> to terminate)\n");

  accel::init();
  accel::begin();

  time_sleep(1); 

  auto *thread = start_thread([](void *) -> void * {
      // NOTE: not testing ADC b/c currently disabled
      while(true) {
        auto acc = accel::get_acceleration();
        auto temp = accel::get_temperature();
        printf("Acceleration: X=%fg\tY=%fg\tZ=%fg\nTemperature: %f°C\n",
            acc.x, acc.y, acc.z, temp.temp);
      }

      return nullptr;
    }, nullptr);

  char c;
  while ((c = getchar()) != '\n' && c != EOF);  // Wait for <Enter> or EOF
  
  stop_thread(thread);

  accel::end();
  accel::close();

  return 0;
}
