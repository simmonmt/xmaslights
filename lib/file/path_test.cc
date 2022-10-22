#include "lib/file/path.h"

#include "gtest/gtest.h"

namespace {

TEST(JoinPathTest, Test) {
  EXPECT_EQ(JoinPath({"a", "b"}), "a/b");
  EXPECT_EQ(JoinPath({"a/", "b"}), "a/b");
}

}  // namespace
