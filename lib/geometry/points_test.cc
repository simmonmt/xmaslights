#include "lib/geometry/points.h"

#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include "lib/geometry/points_testutil.h"

namespace {

using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Pointwise;

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

TEST(MatcherTest, XYZPos) {
  XYZPos one = {.x = 3, .y = 4, .z = 5};
  XYZPos xoff = {.x = 3.004, .y = 4, .z = 5};
  XYZPos yoff = {.x = 3, .y = 4.004, .z = 5};
  XYZPos zoff = {.x = 3, .y = 4, .z = 5.004};
  XYZPos alloff = {.x = 3.004, .y = 4.004, .z = 5.004};

  EXPECT_THAT(one, XYZPosNear(one, 0));
  EXPECT_THAT(one, Not(XYZPosNear(xoff, 0)));
  EXPECT_THAT(one, Not(XYZPosNear(yoff, 0)));
  EXPECT_THAT(one, Not(XYZPosNear(zoff, 0)));
  EXPECT_THAT(one, Not(XYZPosNear(alloff, 0)));

  EXPECT_THAT(one, XYZPosNear(one, 0.005));
  EXPECT_THAT(one, XYZPosNear(xoff, 0.005));
  EXPECT_THAT(one, XYZPosNear(yoff, 0.005));
  EXPECT_THAT(one, XYZPosNear(zoff, 0.005));
  EXPECT_THAT(one, XYZPosNear(alloff, 0.005));

  std::vector<XYZPos> onevec({one});
  EXPECT_THAT(onevec, Pointwise(XYZPosNear(0), std::vector<XYZPos>({one})));
  EXPECT_THAT(onevec,
              Pointwise(XYZPosNear(0.005), std::vector<XYZPos>({xoff})));
}

TEST(PointsIndexTest, Test) {
  PointsIndex index;
  index.Add(XYZPos{1, 2, 3}, 19);
  index.Add(XYZPos{2, 3, 4}, 20);
  index.Add(XYZPos{3, 4, 5}, 21);
  index.Add(XYZPos{4, 5, 6}, 21);
  index.Add(XYZPos{5, 6, 7}, 22);

  EXPECT_THAT(index.Near(20.5, 0.25), IsEmpty());
  EXPECT_THAT(
      index.Near(20.5, 1),
      UnorderedPointwise(XYZPosNear(0.1),
                         {XYZPos{2, 3, 4}, XYZPos{3, 4, 5}, XYZPos{4, 5, 6}}));
}

}  // namespace
