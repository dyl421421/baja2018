#include <cstdio>
#include <cstdlib>

#include <pigpiod_if2.h>

#include "adc.h"
#include "csv.h"

using namespace std;

const char *usage = "Usage: %s [duration in seconds]\n";

int main(int argc, char **argv) {
  if (argc != 2) {
    printf(usage, *argv);
    return -1;
  }

  const double DELAY = 20 / 1e3;  // 20 milliseconds

  double s = strtod(argv[1], NULL);
  Csv adc_csv("adc_csv_test.csv",
      {"Timestamp (µs)", "Cell 1", "Cell 2", "Cell 3"});
  
  adc::init();
  adc::begin();

  // Returns time in seconds
  double start = time_time(), time_offset;

  while ((time_offset = time_time() - start) <= s) {
    auto cell1 = adc::get(adc::CS0, 0),
         cell2 = adc::get(adc::CS0, 1),
         cell3 = adc::get(adc::CS0, 2);

    // Microseconds
    adc_csv << (unsigned long long) (time_offset * 1e6)
      << cell1 << cell2 << cell3 << Csv::LINE_BREAK;

    time_sleep(DELAY - ((time_time() - start) - time_offset));
  }

  adc::end();
  adc::close();

  return 0;
}
