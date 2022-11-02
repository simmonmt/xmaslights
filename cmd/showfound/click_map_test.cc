#include "cmd/showfound/click_map.h"

#include "gtest/gtest.h"

namespace {

TEST(ClickMapTest, Test) {
  cv::Size size;
  size.width = 30;
  size.height = 10;

  ClickMap map(size, {{1, {1, 1}}});

  EXPECT_EQ(1, map.WhichTarget({0, 0}));
  EXPECT_EQ(1, map.WhichTarget({1, 1}));
  EXPECT_EQ(1, map.WhichTarget({6, 1}));
  EXPECT_EQ(-1, map.WhichTarget({7, 1}));
  EXPECT_EQ(1, map.WhichTarget({1, 6}));
  EXPECT_EQ(-1, map.WhichTarget({1, 7}));
}

}  // namespace
