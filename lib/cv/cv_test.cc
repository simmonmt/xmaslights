#include "lib/cv/cv.h"

#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include "lib/file/path.h"
#include "opencv2/viz/types.hpp"

namespace {

TEST(CvTest, CvReadImage) {
  EXPECT_FALSE(CvReadImage(JoinPath({testing::TempDir(), "nonexistent"})).ok());
}

TEST(CvColorToRgbBytes, Test) {
  const std::vector<std::tuple<cv::viz::Color, unsigned long>> test_cases = {
      {cv::viz::Color::white(), 0xffffff},
      {cv::viz::Color::red(), 0xff0000},
      {cv::viz::Color::green(), 0x00ff00},
      {cv::viz::Color::blue(), 0x0000ff},
  };

  for (const auto& [in, want] : test_cases) {
    unsigned long got = CvColorToRgbBytes(in);
    EXPECT_EQ(got, want) << absl::StrFormat("got %06x, want %06x", got, want);
  }
}

}  // namespace
