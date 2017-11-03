#ifndef ADC_H_
#define ADC_H_

#include <cassert>

// Thin wrapper to pigpio spi
namespace adc {
// Call before all calling any other function here (attach to daemon)
void init();
// Call before calling get (establish spi connections)
void begin();
// Call to undo call to begin (destroy spi connections)
void end();
// Call to undo call to init (detatch from daemon)
void close();

enum ChipSelect { CS0, CS1 };

// Get data from a cs and a channel
// cs - ChipSelect
// ch - channel num, must be >= 0 and <= 7
int get(ChipSelect cs, char ch);
}  // namespace adc

#endif  // ADC_H_
