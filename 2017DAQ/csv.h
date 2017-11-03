#ifndef CSV_H_
#define CSV_H_

#include <fstream>
#include <memory>
#include <vector>

class Csv {
 public:  
  // Type for printing just a new line to ofs_
  static const struct LineBreak_t {} LINE_BREAK;

  // Takes ownership of file stream
  Csv(std::unique_ptr<std::ofstream> ofs,
      const std::vector<const char *> &headers);
  Csv(const char *filename, const std::vector<const char *> &headers);

  Csv() = delete;
  Csv(const Csv &) = delete;
  Csv &operator=(const Csv &) = delete;

  // For printing an item to csv
  // Print LINE_BREAK to print just a new line
  template <typename T>
  Csv &operator<<(const T &t);

 private:
  std::unique_ptr<std::ofstream> ofs_;
  bool first_col_ = true;  // Used for controlling spaces
};

// Templated Implementation of print
template <typename T>
inline Csv &Csv::operator<<(const T &t) {
  if (first_col_) {
    first_col_ = false;
  } else {
    *ofs_ << ',';
  }
  *ofs_ << t;

  return *this;
}

// Specialization for LineBreak_t
template <>
inline Csv &Csv::operator<<(const Csv::LineBreak_t &) {
  *ofs_ << std::endl;
  first_col_ = true;
  return *this;
}

#endif  // CSV_H_
