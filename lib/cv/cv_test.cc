#include "lib/cv/cv.h"

#include "gtest/gtest.h"
#include "lib/file/path.h"

namespace {

TEST(CvTest, CvReadImage) {
  EXPECT_FALSE(CvReadImage(JoinPath({testing::TempDir(), "nonexistent"})).ok());
}

}  // namespace
