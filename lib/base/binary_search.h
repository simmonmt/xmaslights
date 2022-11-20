#ifndef _LIB_BASE_BINARY_SEARCH_H_
#define _LIB_BASE_BINARY_SEARCH_H_ 1

#include <functional>

double BinarySearch(double start, double end, double min_step,
                    std::function<int(double)> func);

#endif  // _LIB_BASE_BINARY_SEARCH_H_
