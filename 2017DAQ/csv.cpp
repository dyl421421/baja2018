#include "csv.h"

#include <utility>

using namespace std;

// Instantiate special static LINE_BREAK val
const Csv::LineBreak_t Csv::LINE_BREAK;

Csv::Csv(unique_ptr<ofstream> ofs, const vector<const char *> &headers)
    : ofs_(move(ofs)) {
  for (const char *header : headers) {
    *this << header;
  }

  *this << LINE_BREAK;
}

Csv::Csv(const char *filename, const vector<const char *> &headers)
    : Csv(move(unique_ptr<ofstream>(new ofstream(filename))), headers) {}
