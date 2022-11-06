#include "cmd/showfound/solver.h"

#include <iostream>

#include "absl/debugging/failure_signal_handler.h"
#include "cmd/showfound/model.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/geometry/translation.h"

namespace {

using ::testing::AllOf;
using ::testing::DoubleNear;
using ::testing::Field;

TEST(SynthesizePixelLocationTest, Test) {
  CameraMetadata metadata = {
      .distance_from_center = 176,
      .fov_h = Radians(65),
      .fov_v = Radians(150),
      .res_h = 720,
      .res_v = 1280,
  };

  auto pixels =
      std::make_unique<std::vector<ModelPixel>>(std::vector<ModelPixel>{
          {
              258,
              {{{400, 515}}},
              {{61.7898, 7.207730, 57.9136}},
          },
          {
              196,
              {{{366, 564}}},
              {{69.6723, 1.005240, 40.5136}},
          },
          {
              198,
              {{{405, 577}}},
              {{60.7838, 8.18301, 35.473}},
          },
      });

  std::vector<std::unique_ptr<CameraImages>> camera_images;
  camera_images.push_back(
      CameraImages::CreateWithImages(cv::Mat(), cv::Mat(), "nonexistent"));

  PixelModel model(std::move(camera_images), std::move(pixels),
                   std::make_unique<NopPixelWriter>());
  PixelSolver solver(model, metadata);

  int refs[3] = {258, 196, 198};
  cv::Point3d result = solver.SynthesizePixelLocation(1, {390, 550}, refs);
  std::cout << "result is " << result.x << "," << result.y << "," << result.z
            << "\n";

  EXPECT_THAT(result,
              AllOf(Field("x", &cv::Point3d::x, DoubleNear(64.302, 0.001)),
                    Field("y", &cv::Point3d::y, DoubleNear(5.284, 0.001)),
                    Field("z", &cv::Point3d::z, DoubleNear(45.246, 0.001))));
}

}  // namespace

int main(int argc, char** argv) {
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
