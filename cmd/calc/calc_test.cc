#include "cmd/calc/calc.h"

#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/geometry/points.h"
#include "lib/geometry/points_testutil.h"
#include "lib/geometry/translation.h"

namespace {

using ::testing::AllOf;
using ::testing::DoubleEq;
using ::testing::DoubleNear;
using ::testing::Gt;
using ::testing::Lt;
using ::testing::Not;
using ::testing::PrintToString;

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
  const CameraMetadata metadata = {
      .distance_from_center = 10,
      .fov_h = Radians(90),
      .fov_v = Radians(60),
      .res_h = 720,
      .res_v = 1080,
  };

  auto result = FindDetectionLocation({.x = 400, .y = 400},
                                      {.x = 320, .y = 400}, metadata);
  EXPECT_THAT(result.detection,
              XYZPosNear(XYZPos{0.4808, 0.8328, 1.30517}, 0.0001));
  EXPECT_THAT(result.pixel_y_error, DoubleEq(0));

  result = FindDetectionLocation({.x = 450, .y = 400}, {.x = 320, .y = 410},
                                 metadata);
  EXPECT_THAT(result.detection,
              XYZPosNear(XYZPos{-.3820, 2.0651, 1.23306}, 0.0001));
  EXPECT_THAT(result.pixel_y_error, DoubleEq(10));
}

}  // namespace
