#ifndef _LIB_GEOMETRY_POINTS_TESTUTIL_H_
#define _LIB_GEOMETRY_POINTS_TESTUTIL_H_ 1

#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "lib/geometry/points.h"

MATCHER_P(XYPosEq, want,
          absl::StrFormat("XY Position %s %s", (negation ? "isn't" : "is"),
                          ::testing::PrintToString(want))) {
  return ExplainMatchResult(::testing::DoubleEq(want.x), arg.x,
                            result_listener) &&
         ExplainMatchResult(::testing::DoubleEq(want.y), arg.y,
                            result_listener);
}

MATCHER_P2(XYPosNear, want, max_abs_error,
           absl::StrFormat("XY Position %s near %s",
                           (negation ? "isn't" : "is"),
                           ::testing::PrintToString(want))) {
  return ExplainMatchResult(::testing::DoubleNear(want.x, max_abs_error), arg.x,
                            result_listener) &&
         ExplainMatchResult(::testing::DoubleNear(want.y, max_abs_error), arg.y,
                            result_listener);
}

::testing::Matcher<std::tuple<XYZPos, XYZPos>> XYZPosNear(double within);
::testing::Matcher<XYZPos> XYZPosNear(const XYZPos& want, double within);

#endif  // _LIB_GEOMETRY_POINTS_TESTUTIL_H_
