#include "lib/strings/strutil.h"

#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/base/streams.h"

namespace {

using ::testing::Eq;

TEST(IndexesToRangesTest, Test) {
  const std::vector<std::tuple<std::vector<int>, std::string>> test_cases = {
      {std::vector<int>(), ""}, {{1}, "1"},
      {{1, 2}, "1-2"},          {{1, 2, 3}, "1-3"},
      {{1, 3, 4, 5}, "1,3-5"},  {{1, 3, 4, 5, 9}, "1,3-5,9"},
  };

  for (const auto& [in, want] : test_cases) {
    std::stringstream in_str;
    in_str << in;

    EXPECT_THAT(IndexesToRanges(in), Eq(want)) << in_str.str();
  }
}

}  // namespace
