#include "csv.h"

int main(int argc, char **argv) {
  Csv test_csv("csv_test.csv", {"Col 1 (i)", "Col 2 (i*i)", "Col 3 ('a' + i)"});

  for (int i = 0; i < 10; ++i) {
    test_csv << i << i*i << (char)('a' + i) << Csv::LINE_BREAK;
  }

  return 0;
}
