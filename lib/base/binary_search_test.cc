#include "lib/base/binary_search.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::testing::ContainerEq;

TEST(BinarySearchTest, Test) {
  struct TestCase {
    double start;
    double end;
    double min_step;
    double want;
    std::vector<double> expected_steps;
    double expected_end;
  };

  const TestCase test_cases[] = {
      {.start = 0,
       .end = 8,
       .min_step = 1,
       .want = 1,
       .expected_steps = {4, 2, 1},
       .expected_end = 1},

      {.start = -10,
       .end = 10,
       .min_step = 1,
       .want = 3,
       .expected_steps = {0, 5, 2.5, 3.75},
       .expected_end = 3.125},
  };

  for (const TestCase& test_case : test_cases) {
    std::vector<double> steps;
    auto func = [&](double loc) {
      steps.push_back(loc);
      std::cerr << "MTS called with loc " << loc << " ret "
                << test_case.want - loc << "\n";
      if (double diff = test_case.want - loc; diff < 0) {
        return -1;
      } else if (diff == 0) {
        return 0;
      } else {
        return 1;
      }
    };

    EXPECT_EQ(
        BinarySearch(test_case.start, test_case.end, test_case.min_step, func),
        test_case.expected_end);
    EXPECT_THAT(steps, ContainerEq(test_case.expected_steps));
  }
}

}  // namespace
