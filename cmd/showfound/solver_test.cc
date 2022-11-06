#include "cmd/showfound/solver.h"

#include <iostream>

#include "cmd/showfound/model.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/geometry/translation.h"
#include "lib/testing/proto.h"
#include "proto/points.pb.h"

namespace {

using ::testing::AllOf;
using ::testing::DoubleNear;
using ::testing::Field;

TEST(SolverTest, CalculateWorldLocation) {
  const CameraMetadata metadata = {
      .distance_from_center = 10,
      .fov_h = Radians(90),
      .fov_v = Radians(60),
      .res_h = 720,
      .res_v = 1080,
  };

  auto pixels =
      std::make_unique<std::vector<ModelPixel>>(std::vector<ModelPixel>{
          ModelPixel(ParseTextProtoOrDie<proto::PixelRecord>(
              "pixel_number: 1 "
              "camera_pixel { "
              "  camera_number: 1 "
              "  pixel_location { x: 400 y: 400 } "
              "} "
              "camera_pixel { "
              "  camera_number: 2 "
              "  pixel_location { x: 320 y: 400 } "
              "}")),
          ModelPixel(ParseTextProtoOrDie<proto::PixelRecord>(
              "pixel_number: 2 "
              "camera_pixel { "
              "  camera_number: 1 "
              "  pixel_location { x: 450 y: 400 } "
              "} "
              "camera_pixel { "
              "  camera_number: 2 "
              "  pixel_location { x: 320 y: 410 } "
              "}")),
      });

  std::vector<std::unique_ptr<CameraImages>> camera_images;
  camera_images.push_back(
      CameraImages::CreateWithImages(cv::Mat(), cv::Mat(), "nonexistent"));

  PixelModel model(std::move(camera_images), std::move(pixels),
                   std::make_unique<NopPixelWriter>());
  PixelSolver solver(model, metadata);

  EXPECT_THAT(solver.CalculateWorldLocation(*model.FindPixel(1)),
              AllOf(Field("x", &cv::Point3d::x, DoubleNear(0.4808, 0.0001)),
                    Field("y", &cv::Point3d::y, DoubleNear(0.8328, 0.0001)),
                    Field("z", &cv::Point3d::z, DoubleNear(1.3052, 0.0001))));

  EXPECT_THAT(solver.CalculateWorldLocation(*model.FindPixel(2)),
              AllOf(Field("x", &cv::Point3d::x, DoubleNear(-.3820, 0.0001)),
                    Field("y", &cv::Point3d::y, DoubleNear(2.0651, 0.0001)),
                    Field("z", &cv::Point3d::z, DoubleNear(1.2331, 0.0001))));
}

TEST(SolverTest, SynthesizePixelLocation) {
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
