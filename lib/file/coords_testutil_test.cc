#include "lib/file/coords_testutil.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/file/coords.h"

namespace {

using ::testing::Not;

TEST(CoordsRecordEqTest, PixelNumber) {
  CoordsRecord a = {.pixel_num = 1};
  CoordsRecord b = {.pixel_num = 2};

  EXPECT_THAT(a, CoordsRecordEq(a, 0));
  EXPECT_THAT(b, CoordsRecordEq(b, 0));
  EXPECT_THAT(a, Not(CoordsRecordEq(b, 0)));
  EXPECT_THAT(b, Not(CoordsRecordEq(a, 0)));
}

TEST(CoordsRecordEqTest, CameraCoords) {
  CoordsRecord a = {
      .pixel_num = 1,
      .camera_coords =
          {
              {{1, 2}},
              std::nullopt,
          },
  };
  CoordsRecord b = {
      .pixel_num = 1,
      .camera_coords =
          {
              std::nullopt,
              {{1, 2}},
          },
  };

  EXPECT_THAT(a, CoordsRecordEq(a, 0));
  EXPECT_THAT(b, CoordsRecordEq(b, 0));
  EXPECT_THAT(a, Not(CoordsRecordEq(b, 0)));
  EXPECT_THAT(b, Not(CoordsRecordEq(a, 0)));
}

TEST(CoordsRecordEqTest, WorldCoords) {
  CoordsRecord a = {
      .pixel_num = 1,
      .world_coord = {{1, 2, 3}},
  };
  CoordsRecord b = {
      .pixel_num = 1,
      .world_coord = {{4, 5, 6}},
  };
  CoordsRecord none = {
      .pixel_num = 1,
      .world_coord = std::nullopt,
  };

  EXPECT_THAT(a, CoordsRecordEq(a, 0));
  EXPECT_THAT(b, CoordsRecordEq(b, 0));
  EXPECT_THAT(none, CoordsRecordEq(none, 0));

  EXPECT_THAT(a, Not(CoordsRecordEq(b, 0)));
  EXPECT_THAT(a, Not(CoordsRecordEq(none, 0)));

  EXPECT_THAT(b, Not(CoordsRecordEq(a, 0)));
  EXPECT_THAT(b, Not(CoordsRecordEq(none, 0)));

  EXPECT_THAT(none, Not(CoordsRecordEq(a, 0)));
  EXPECT_THAT(none, Not(CoordsRecordEq(b, 0)));
}

}  // namespace
