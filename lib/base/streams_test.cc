#include "lib/base/streams.h"

#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "opencv2/core/types.hpp"

namespace {

TEST(StreamsTest, CvPoint) {
  cv::Point2i p(10, 20);
  std::stringstream ss;
  ss << p;
  EXPECT_EQ(ss.str(), "10,20") << p;
  EXPECT_THAT(ss.str(), "10,20") << p;
}

}  // namespace
