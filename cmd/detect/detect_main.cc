#include <iostream>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "cmd/detect/detect.h"
#include "lib/file/file.h"
#include "lib/file/path.h"
#include "lib/file/proto.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"
#include "proto/points.pb.h"

ABSL_FLAG(std::string, on_file, "", "On file");
ABSL_FLAG(std::string, off_file, "", "Off file");
ABSL_FLAG(std::string, intermediates_dir, "",
          "Directory for intermediate results");
ABSL_FLAG(bool, show_result, false, "imshow result");
ABSL_FLAG(int, camera_number, -1, "Camera number");
ABSL_FLAG(int, pixel_number, -1, "Pixel number");
ABSL_FLAG(std::string, input_coords, "",
          "File containing coordinates in proto.PixelRecords textproto format");
ABSL_FLAG(std::string, output_coords, "",
          "File containing coordinates in proto.PixelRecords textproto format");

namespace {

absl::Status SaveImage(cv::Mat mat, const std::string& filename) {
  if (!cv::imwrite(filename, mat)) {
    return absl::UnknownError(
        absl::StrCat("failed to write image to ", filename));
  }
  return absl::OkStatus();
}

absl::StatusOr<std::unique_ptr<proto::PixelRecords>> ReadPixelsFromProto(
    const std::string& path) {
  auto records = std::make_unique<proto::PixelRecords>();
  if (absl::Status status = ReadProto(path, records.get()); !status.ok()) {
    return status;
  }
  return records;
}

void InsertResult(int camera_num, int pixel_num,
                  std::optional<cv::Point2i> point,
                  proto::PixelRecords* pixels) {
  for (proto::PixelRecord& pixel : *pixels->mutable_pixel()) {
    if (pixel.pixel_number() == pixel_num) {
      if (point.has_value()) {
        bool added = false;
        for (proto::CameraPixelLocation& camera :
             *pixel.mutable_camera_pixel()) {
          if (camera.camera_number() == camera_num) {
            camera.mutable_pixel_location()->set_x(point->x);
            camera.mutable_pixel_location()->set_y(point->y);
            camera.clear_manually_adjusted();
            added = true;
            break;
          }
        }

        if (!added) {
          proto::CameraPixelLocation* camera = pixel.add_camera_pixel();
          camera->set_camera_number(camera_num);
          camera->mutable_pixel_location()->set_x(point->x);
          camera->mutable_pixel_location()->set_y(point->y);
        }
      }
      return;
    }
  }

  proto::PixelRecord* pixel = pixels->add_pixel();
  pixel->set_pixel_number(pixel_num);
  if (point.has_value()) {
    proto::CameraPixelLocation* camera = pixel->add_camera_pixel();
    camera->set_camera_number(camera_num);
    camera->mutable_pixel_location()->set_x(point->x);
    camera->mutable_pixel_location()->set_y(point->y);
  }
}

}  // namespace

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetProgramUsageMessage("detects pixels in images");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  const int camera_num = absl::GetFlag(FLAGS_camera_number);
  QCHECK_GT(camera_num, 0) << "--camera_number is required";
  const int pixel_num = absl::GetFlag(FLAGS_pixel_number);
  QCHECK_GE(pixel_num, 0) << "--pixel_number is required";

  auto coords = std::make_unique<proto::PixelRecords>();
  if (const std::string& path = absl::GetFlag(FLAGS_input_coords);
      !path.empty() && Exists(path).value_or(false)) {
    auto status = ReadPixelsFromProto(path);
    QCHECK_OK(status);
    coords = std::move(*status);
  }

  QCHECK(coords == nullptr || !absl::GetFlag(FLAGS_output_coords).empty())
      << "--output_coords is required if --input_coords is passed";

  QCHECK(!absl::GetFlag(FLAGS_on_file).empty()) << "--on_file is required";
  QCHECK(!absl::GetFlag(FLAGS_off_file).empty()) << "--off_file is required";

  LOG(INFO) << "off is " << absl::GetFlag(FLAGS_off_file);
  LOG(INFO) << "on  is " << absl::GetFlag(FLAGS_on_file);

  cv::Mat on_image = cv::imread(absl::GetFlag(FLAGS_on_file));
  cv::Mat off_image = cv::imread(absl::GetFlag(FLAGS_off_file));

  QCHECK_EQ(on_image.rows, off_image.rows) << "size mismatch";
  QCHECK_EQ(on_image.cols, off_image.cols) << "size mismatch";

  cv::Mat blank(on_image.rows, on_image.cols, CV_8U, cv::Scalar::all(0));
  cv::Mat mask;
  cv::bitwise_not(blank, mask);

  std::unique_ptr<DetectResults> results = Detect(off_image, on_image, mask);

  if (!absl::GetFlag(FLAGS_intermediates_dir).empty()) {
    const std::string& dir = absl::GetFlag(FLAGS_intermediates_dir);
    QCHECK_OK(SaveImage(on_image, JoinPath({dir, "on.jpg"})));
    QCHECK_OK(SaveImage(off_image, JoinPath({dir, "off.jpg"})));

    for (const auto& elem : results->intermediates) {
      const std::string& name = elem.first;
      const cv::Mat& image = elem.second;
      QCHECK_OK(SaveImage(image, JoinPath({dir, name + ".jpg"})));
    }
  }

  if (!results->found) {
    if (const std::string path = absl::GetFlag(FLAGS_output_coords);
        !path.empty()) {
      InsertResult(camera_num, pixel_num, std::nullopt, coords.get());
      QCHECK_OK(WriteTextProto(path, *coords));
    }
    return 1;
  }

  if (absl::GetFlag(FLAGS_show_result)) {
    cv::imshow("Result", results->intermediates["marked"]);
    cv::waitKey(0);
  }

  LOG(INFO) << absl::StrFormat("%d,%d\n", results->centroid.x,
                               results->centroid.y);

  if (const std::string path = absl::GetFlag(FLAGS_output_coords);
      !path.empty()) {
    InsertResult(camera_num, pixel_num, results->centroid, coords.get());
    QCHECK_OK(WriteTextProto(path, *coords));
  }

  return 0;
}
