#include "lib/base/binary_search.h"

#include <iostream>

double BinarySearch(double start, double end, double min_step,
                    std::function<int(double)> func) {
  double left = start, right = end;
  for (;;) {
    double step = (right - left) / 2;
    double cur = left + step;
    if (step < min_step) {
      std::cerr << "step is " << step << "\n";
      return cur;
    }

    if (int rc = func(cur); rc < 0) {
      right = cur;
    } else if (rc > 0) {
      left = cur;
    } else {  // rc == 0
      return cur;
    }
  }
}
