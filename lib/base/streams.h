#ifndef _LIB_BASE_STREAMS_H_
#define _LIB_BASE_STREAMS_H_ 1

#include <iostream>

#include "opencv2/core/types.hpp"

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
  os << "[";
  for (int i = 0; i < vec.size(); ++i) {
    if (i != 0) {
      os << ",";
    }
    os << vec[i];
  }
  return os << "]";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const cv::Point_<T>& loc) {
  os << loc.x;
  os << ",";
  os << loc.y;
  return os;
}

#endif  // _LIB_BASE_STREAMS_H_
