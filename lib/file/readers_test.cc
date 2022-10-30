#include "lib/file/readers.h"

#include <string>

#include "absl/log/check.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/file/path.h"
#include "lib/file/writers.h"

namespace {

using ::testing::ElementsAre;

std::string Path(const std::string& relpath) {
  return JoinPath({::testing::TempDir(), relpath});
}

TEST(ReadInts, Error) { ASSERT_FALSE(ReadInts(Path("nonexistent")).ok()); }

TEST(ReadInts, Reading) {
  const std::string path = Path("output");
  QCHECK_OK(WriteFile(path, {"1", "2", "3"}));

  absl::StatusOr<std::vector<int>> ints = ReadInts(path);
  ASSERT_TRUE(ints.ok());
  EXPECT_THAT(*ints, ElementsAre(1, 2, 3));
}

}  // namespace
