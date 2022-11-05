#ifndef _LIB_BASE_STREAMS_H_
#define _LIB_BASE_STREAMS_H_ 1

#include <iostream>

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

#endif  // _LIB_BASE_STREAMS_H_
