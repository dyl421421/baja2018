#include "ir_temp.h"

#include <pigpiod_if2.h>
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
  cout << "Temperature Output (Press <Enter> to terminate):" << endl;
  cout << "Format: DEVICE: OBJ_TEMP (AMB_TEMP) ..." << endl;
  
  ir_temp::init();
  ir_temp::begin();

  time_sleep(1);

  auto *thread = start_thread([](void *) -> void * {
      while (true) {
        cout
          //<< "CVT Belt: " << ir_temp::get_obj(ir_temp::CVT_BELT).val << "°F ("
          //<< ir_temp::get_amb(ir_temp::CVT_BELT).val << "°F)\t"
          << "R Rotor: " << ir_temp::get_obj(ir_temp::R_ROTOR).val << "°F ("
          << ir_temp::get_amb(ir_temp::R_ROTOR).val << "°F)" << endl;
      }

      return nullptr;
    }, nullptr);

  while (cin.get() != '\n');  // Wait for enter

  stop_thread(thread);

  ir_temp::end();
  ir_temp::close();
  
  return 0;
}
