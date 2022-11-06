#ifndef _LIB_TESTING_CV_H_
#define _LIB_TESTING_CV_H_ 1

#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "lib/base/streams.h"

MATCHER_P(CvPointEq, want, absl::StrCat("point is ", want.x, ",", want.y)) {
  *result_listener << "where the value is " << arg.x << "," << arg.y;
  return want.x == arg.x && want.y == arg.y;
};

#endif  // _LIB_TESTING_CV_H_
