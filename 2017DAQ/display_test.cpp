#include <iostream>
#include <thread>
#include <unistd.h>

#include "display.h"

using namespace std;

const int kSleepTimeMicro = 330000;  // .33s

void cycle(int rpm, int mph, int flags) {
  cout << "RPM: " << rpm << "\tMPH: " << mph  << "\tSTAT:";
  if (flags & display::INFO_BRAKE)
    cout << " INFO BRAKE";
  if (flags & display::WARNING_TEMP)
    cout << " WARN TEMP";
  if (flags & display::WARNING_BATTERY)
    cout << " WARN BATTERY";
  if (flags & display::INFO_DATA_LOGGING)
    cout << " INFO DATA LOG";
  if (flags & display::STATUS_UNKNOWN)
    cout << " UNKNOWN";
  if (!flags)
    cout << " NONE";
  cout << endl;

  display::update(rpm, mph, flags);

  usleep(kSleepTimeMicro);
}

void display_loop() {
  while (true) {
    cycle(4000, 88, display::WARNING_BATTERY | display::INFO_BRAKE | 
        display::WARNING_TEMP | display::INFO_DATA_LOGGING);

    cycle(0, 88, display::WARNING_BATTERY);
    cycle(0, 88, display::WARNING_TEMP);
    cycle(0, 88, display::INFO_BRAKE);
    cycle(0, 88, display::INFO_DATA_LOGGING);

    for (int i = 0; i < 10; ++i) {
      cycle(0, i*10 + i, display::STATUS_NONE);
    }

    for (int i = 0; i < 4000; i += 4000/12)
      cycle(i, 0, display::STATUS_NONE);

    cycle(0, 0, display::STATUS_NONE);
  }
}

int main(int argc, char **argv) {
  display::init();
  display::begin();
  
  cout << "Press <ENTER> to terminate test" << endl;
  usleep(kSleepTimeMicro);

  {  // Scope for display_thread
    thread display_thread(&display_loop);
    display_thread.detach();

    while (cin.get() != '\n');  // Wait for enter
  }

  display::end();
  display::close();

  return 0;
}
