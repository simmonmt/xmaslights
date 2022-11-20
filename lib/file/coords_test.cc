#include "lib/file/coords.h"

#include <optional>
#include <string>

#include "absl/log/check.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/file/coords_testutil.h"
#include "lib/file/path.h"
#include "lib/file/writers.h"

namespace {

using ::testing::Pointwise;
using ::testing::SizeIs;

class CoordsTest : public ::testing::Test {
 public:
  std::string Path(const std::string& relpath) {
    return JoinPath({::testing::TempDir(), relpath});
  }
};

TEST_F(CoordsTest, Errors) {
  EXPECT_FALSE(ReadCoords(Path("nonexistent"), std::nullopt).ok());
  EXPECT_FALSE(ReadCoords(std::nullopt, Path("nonexistent")).ok());
  EXPECT_FALSE(ReadCoords(std::nullopt, std::nullopt).ok());
}

TEST_F(CoordsTest, CameraCoords) {
  const std::string merged_path = Path("merged");
  QCHECK_OK(
      WriteFile(merged_path, {"1 10,20 -", "2 - 30,40", "3 50,60 70,80"}));

  absl::StatusOr<std::vector<CoordsRecord>> result =
      ReadCoords(merged_path, std::nullopt);
  ASSERT_TRUE(result.ok());
}

TEST_F(CoordsTest, WorldCoords) {
  const std::string world_path = Path("world");
  QCHECK_OK(WriteFile(world_path, {"1 10,20,30", "2 40,50,60"}));

  absl::StatusOr<std::vector<CoordsRecord>> result =
      ReadCoords(std::nullopt, world_path);
  ASSERT_TRUE(result.ok());
  EXPECT_THAT(*result, SizeIs(2));
}

TEST_F(CoordsTest, Full) {
  const std::string merged_path = Path("merged");
  QCHECK_OK(
      WriteFile(merged_path, {"1 10,20 -", "2 - 30,40", "3 50,60 70,80"}));

  const std::string world_path = Path("world");
  QCHECK_OK(WriteFile(world_path, {"1 1.1,2.2,3.3", "4 4.4,5.5,6.6"}));

  std::vector<CoordsRecord> want = {
      {.pixel_num = 1,
       .camera_coords = {{{10, 20}}, std::nullopt},
       .world_coord = {{1.1, 2.2, 3.3}}},
      {.pixel_num = 2,
       .camera_coords = {std::nullopt, {{30, 40}}},
       .world_coord = std::nullopt},
      {.pixel_num = 3,
       .camera_coords = {{{50, 60}}, {{70, 80}}},
       .world_coord = std::nullopt},
      {.pixel_num = 4, .world_coord = {{4.4, 5.5, 6.6}}},
  };

  absl::StatusOr<std::vector<CoordsRecord>> result =
      ReadCoords(merged_path, world_path);
  ASSERT_TRUE(result.ok());
  EXPECT_THAT(*result, Pointwise(CoordsRecordEq(0.001), want));
}

}  // namespace
