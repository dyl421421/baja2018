CXX		= g++
CXXFLAGS	= -std=c++11 -pthread -Wall -Wpedantic
LDFLAGS		= -lpigpiod_if2 -lrt -lm
TARGETS		= driver
TESTS		= adc display csv adc_csv ir_temp accel
BIN_DIR		= /home/pi/bin/

.PHONY: all test debug clean

all: CXXFLAGS += -O3 -DNDEBUG
all: $(addprefix $(BIN_DIR)/, $(TARGETS))

debug: CXXFLAGS += -g
debug: $(addprefix $(BIN_DIR)/, $(TARGETS))

test: CXXFLAGS += -g -DNDEBUG
test: $(addsuffix _test, $(TESTS))
	
display_test: display_test.o display.o display.h util.h
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

adc_test: adc_test.o adc.o adc.h util.h
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

csv_test: csv_test.o csv.o csv.h
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

adc_csv_test: adc_csv_test.o adc.o csv.o adc.h csv.h util.h
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

ir_temp_test: ir_temp_test.o ir_temp.o ir_temp.h util.h
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

accel_test: accel_test.o accel.o util.h
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)

$(BIN_DIR)/driver: driver.o adc.o csv.o accel.o sensors.o ir_temp.o display.o
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LDFLAGS)
	sudo chown root $@
	sudo chmod u=rwx,g=sx,o=sx $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $+

clean:
	rm -rf $(addprefix $(BIN_DIR)/, $(TARGETS)) $(addsuffix _test, $(TESTS)) *.o csv_test.csv adc_csv_test.csv
