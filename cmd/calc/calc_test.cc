#include "cmd/calc/calc.h"

#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::testing::AllOf;
using ::testing::DoubleEq;
using ::testing::DoubleNear;
using ::testing::Gt;
using ::testing::Lt;
using ::testing::Not;
using ::testing::PrintToString;

MATCHER_P(XYPosEq, want,
          absl::StrFormat("XY Position %s %s", (negation ? "isn't" : "is"),
                          PrintToString(want))) {
  return ExplainMatchResult(DoubleEq(want.x), arg.x, result_listener) &&
         ExplainMatchResult(DoubleEq(want.y), arg.y, result_listener);
}

MATCHER_P2(XYPosNear, want, max_abs_error,
           absl::StrFormat("XY Position %s near %s",
                           (negation ? "isn't" : "is"), PrintToString(want))) {
  return ExplainMatchResult(DoubleNear(want.x, max_abs_error), arg.x,
                            result_listener) &&
         ExplainMatchResult(DoubleNear(want.y, max_abs_error), arg.y,
                            result_listener);
}

TEST(MatcherTest, XYPos) {
  XYPos one = {.x = 3, .y = 4};
  XYPos xoff = {.x = 3.004, .y = 4};
  XYPos yoff = {.x = 3, .y = 4.004};
  XYPos bothoff = {.x = 3.004, .y = 4.004};

  EXPECT_THAT(one, XYPosEq(one));
  EXPECT_THAT(one, Not(XYPosEq(xoff)));
  EXPECT_THAT(one, Not(XYPosEq(yoff)));
  EXPECT_THAT(one, Not(XYPosEq(bothoff)));

  EXPECT_THAT(one, XYPosNear(one, 0));
  EXPECT_THAT(one, Not(XYPosNear(xoff, 0)));
  EXPECT_THAT(one, Not(XYPosNear(yoff, 0)));
  EXPECT_THAT(one, Not(XYPosNear(bothoff, 0)));

  EXPECT_THAT(one, XYPosNear(one, 0.005));
  EXPECT_THAT(one, XYPosNear(xoff, 0.005));
  EXPECT_THAT(one, XYPosNear(yoff, 0.005));
  EXPECT_THAT(one, XYPosNear(bothoff, 0.005));
}

class CalcTest : public ::testing::Test {};

TEST_F(CalcTest, FindC1XYPos) {
  EXPECT_THAT(FindC1XYPos(10), XYPosEq(XYPos{.x = 10, .y = 0}));
}

TEST_F(CalcTest, FindC2XYPos) {
  XYPos pos = FindC2XYPos(10);
  EXPECT_THAT(pos, XYPosNear(XYPos{.x = -5, .y = 8.66}, 0.001));
  EXPECT_THAT(FindDist({0, 0}, pos), DoubleEq(10));
}

TEST_F(CalcTest, FindAngleRad) {
  const double fov = M_PI_2;  // 90 degrees
  constexpr int res = 720;
  constexpr double pixel_angle = (M_PI_4 * 2) / res;

  EXPECT_THAT(FindAngleRad(0, res, fov), DoubleEq(-M_PI_4));
  EXPECT_THAT(FindAngleRad(1, res, fov), DoubleEq(-M_PI_4 + pixel_angle));

  // We're calculating the left side of the pixel, so it'll be
  // 1*pixel_angle short of the full sweep.
  EXPECT_THAT(FindAngleRad(719, res, fov), DoubleEq(M_PI_4 - pixel_angle));

  EXPECT_THAT(FindAngleRad(359, res, fov), DoubleNear(0 - pixel_angle, 0.0001));
  EXPECT_THAT(FindAngleRad(360, res, fov), DoubleEq(0.0));
}

TEST_F(CalcTest, FindDist) {
  EXPECT_THAT(FindDist({.x = 10, .y = 20}, {.x = 14, .y = 23}), DoubleEq(5));
}

TEST_F(CalcTest, FindDetectionZ) {
  EXPECT_THAT(FindDetectionZ(M_PI_4, {.x = 10, .y = 20}, {.x = 14, .y = 23}),
              DoubleEq(5));
}

TEST_F(CalcTest, FindLineB) {
  EXPECT_THAT(FindLineB(2, {.x = 3, .y = 2}), DoubleEq(-4));
  EXPECT_THAT(FindLineB(-6, {.x = 3, .y = 2}), DoubleEq(20));
}

TEST_F(CalcTest, FindXYIntersection) {
  EXPECT_THAT(FindXYIntersection({.slope = 2, .b = -4}, {.slope = -6, .b = 20}),
              XYPosEq(XYPos{.x = 3, .y = 2}));
}

TEST_F(CalcTest, FindDetectionLocation) {
  constexpr double D = 10;
  const Metadata metadata = {
      .fov_h = Radians(90),
      .fov_v = Radians(60),
      .res_h = 720,
      .res_v = 1080,
  };

  auto result = FindDetectionLocation(D, {.x = 400, .y = 400},
                                      {.x = 320, .y = 400}, metadata);
  EXPECT_THAT(result.detection_x, DoubleNear(0.4808, 0.0001));
  EXPECT_THAT(result.detection_y, DoubleNear(0.8328, 0.0001));
  EXPECT_THAT(result.detection_z, DoubleNear(1.30517, 0.0001));
  EXPECT_THAT(result.pixel_y_error, DoubleEq(0));

  result = FindDetectionLocation(D, {.x = 450, .y = 400}, {.x = 320, .y = 410},
                                 metadata);
  EXPECT_THAT(result.detection_x, DoubleNear(-.3820, 0.0001));
  EXPECT_THAT(result.detection_y, DoubleNear(2.0651, 0.0001));
  EXPECT_THAT(result.detection_z, DoubleNear(1.23306, 0.0001));
  EXPECT_THAT(result.pixel_y_error, DoubleEq(10));
}

}  // namespace
