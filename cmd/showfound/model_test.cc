#include "cmd/showfound/model.h"

#include "cmd/showfound/camera_images.h"
#include "gtest/gtest.h"
#include "lib/testing/proto.h"

namespace {

std::vector<std::unique_ptr<CameraImages>> MakeCameraImages(int num_cameras) {
  std::vector<std::unique_ptr<CameraImages>> out;
  for (int i = 1; i <= num_cameras; ++i) {
    out.push_back(
        CameraImages::CreateWithImages(cv::Mat(), cv::Mat(), "/nowhere"));
  }
  return out;
}

TEST(PixelModelTest, UpdatePixel) {
  auto pixels = std::make_unique<std::vector<ModelPixel>>(
      std::initializer_list<ModelPixel>{
          ParseTextProtoOrDie<proto::PixelRecord>("pixel_number: 1"),
          ParseTextProtoOrDie<proto::PixelRecord>("pixel_number: 2"),
          ParseTextProtoOrDie<proto::PixelRecord>("pixel_number: 3"),
      });

  PixelModel model(MakeCameraImages(2), std::move(pixels),
                   std::make_unique<NopPixelWriter>());

  ModelPixel update1(ParseTextProtoOrDie<proto::PixelRecord>(
      "pixel_number: 1 camera_pixel { camera_number: 1 }"));
  ASSERT_TRUE(model.UpdatePixel(update1.num(), update1));

  std::string diffs;
  EXPECT_TRUE(ProtoDiff(update1.ToProto(),
                        model.FindPixel(update1.num())->ToProto(), &diffs))
      << diffs;

  ModelPixel update9(ParseTextProtoOrDie<proto::PixelRecord>(
      "pixel_number: 9 camera_pixel { camera_number: 1 }"));
  EXPECT_FALSE(model.UpdatePixel(update9.num(), update9));
}

}  // namespace
