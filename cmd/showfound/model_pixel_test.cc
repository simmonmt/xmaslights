#include "cmd/showfound/model_pixel.h"

#include "absl/log/check.h"
#include "absl/strings/ascii.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "lib/testing/proto.h"
#include "proto/points.pb.h"

namespace {

TEST(ModelPixelBuilderTest, SetCameraLocation) {
  std::string diffs;
  ModelPixel pixel = ModelPixelBuilder(proto::PixelRecord())
                         .SetCameraLocation(3, {1, 2}, false)
                         .Build();

  auto want = ParseTextProtoOrDie<proto::PixelRecord>(
      "camera_pixel { "
      "  camera_number: 3 "
      "  pixel_location { x: 1 y: 2 } "
      "}");
  EXPECT_TRUE(ProtoDiff(want, pixel.ToProto(), &diffs)) << diffs;

  pixel = ModelPixelBuilder(pixel).SetCameraLocation(3, {4, 5}, true).Build();

  want = ParseTextProtoOrDie<proto::PixelRecord>(
      "camera_pixel { "
      "  camera_number: 3 "
      "  pixel_location { x: 4 y: 5 } "
      "  manually_adjusted: true "
      "}");
  EXPECT_TRUE(ProtoDiff(want, pixel.ToProto(), &diffs)) << diffs;

  pixel = ModelPixelBuilder(pixel).SetCameraLocation(1, {1, 2}, false).Build();

  want = ParseTextProtoOrDie<proto::PixelRecord>(
      "camera_pixel { "
      "  camera_number: 1 "
      "  pixel_location { x: 1 y: 2 } "
      "} "
      "camera_pixel { "
      "  camera_number: 3 "
      "  pixel_location { x: 4 y: 5 } "
      "  manually_adjusted: true "
      "}");
  EXPECT_TRUE(ProtoDiff(want, pixel.ToProto(), &diffs)) << diffs;
}

TEST(ModelPixelBuilderTest, SetWorldLocation) {
  std::string diffs;
  ModelPixel pixel = ModelPixelBuilder(proto::PixelRecord())
                         .SetWorldLocation({1, 2, 3})
                         .Build();

  auto want = ParseTextProtoOrDie<proto::PixelRecord>(
      "world_pixel { "
      "  pixel_location { x: 1 y: 2 z: 3 } "
      "}");
  EXPECT_TRUE(ProtoDiff(want, pixel.ToProto(), &diffs)) << diffs;
}

}  // namespace
