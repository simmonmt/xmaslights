#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_set>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "cmd/showfound/camera_images.h"
#include "cmd/showfound/controller.h"
#include "cmd/showfound/model.h"
#include "cmd/showfound/view.h"
#include "lib/file/proto.h"
#include "lib/file/readers.h"
#include "opencv2/opencv.hpp"
#include "proto/camera_metadata.pb.h"
#include "proto/points.pb.h"

ABSL_FLAG(std::string, camera_dirs, "",
          "Comma-separated paths to camera directories");
ABSL_FLAG(std::string, input_coords, "",
          "File containing coordinates in proto.PixelRecords textproto format");
ABSL_FLAG(std::string, output_coords, "",
          "File containing coordinates in proto.PixelRecords textproto format");
ABSL_FLAG(std::string, world_coords, "",
          "Path to file containing list of world coordinates");
ABSL_FLAG(std::string, camera_metadata, "",
          "File containing CameraMetadata textproto");

namespace {

std::vector<std::unique_ptr<CameraImages>> MakeCameraImages(
    const std::string& paths) {
  std::vector<std::unique_ptr<CameraImages>> out;
  std::vector<std::string> parts = absl::StrSplit(paths, ",");
  for (const std::string& path : parts) {
    auto images = CameraImages::Create(path);
    QCHECK_OK(images.status()) << path;
    out.push_back(std::move(*images));
  }
  return out;
}

absl::StatusOr<std::unique_ptr<std::vector<ModelPixel>>> ReadPixelsFromProto(
    const std::string& path) {
  proto::PixelRecords records;
  if (absl::Status status = ReadProto(path, &records); !status.ok()) {
    return status;
  }

  auto pixels = std::make_unique<std::vector<ModelPixel>>();
  for (const proto::PixelRecord& pixel : records.pixel()) {
    pixels->push_back(ModelPixel(pixel));
  }
  return pixels;
}

}  // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("show mapped pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  constexpr int kNumCameras = 2;
  constexpr int kStartCameraNum = 1;

  QCHECK(!absl::GetFlag(FLAGS_camera_metadata).empty())
      << "--camera_metadata is required";
  const CameraMetadata camera_metadata = [&]() {
    auto result =
        ReadProto<proto::CameraMetadata>(absl::GetFlag(FLAGS_camera_metadata));
    QCHECK_OK(result);
    return CameraMetadata::FromProto(*result);
  }();

  QCHECK(!absl::GetFlag(FLAGS_camera_dirs).empty())
      << "--ref_images is required";
  std::vector<std::unique_ptr<CameraImages>> camera_images =
      MakeCameraImages(absl::GetFlag(FLAGS_camera_dirs));
  QCHECK_EQ(camera_images.size(), kNumCameras)
      << "num cameras camera images mismatch";

  QCHECK(!absl::GetFlag(FLAGS_input_coords).empty())
      << "--input_coords is required";
  std::unique_ptr<std::vector<ModelPixel>> pixels = [&] {
    auto status = ReadPixelsFromProto(absl::GetFlag(FLAGS_input_coords));
    QCHECK_OK(status);
    return std::move(*status);
  }();

  QCHECK(!absl::GetFlag(FLAGS_output_coords).empty())
      << "--output_coords is required";
  auto pixel_writer =
      std::make_unique<FilePixelWriter>(absl::GetFlag(FLAGS_output_coords));

  PixelModel model(std::move(camera_images), std::move(pixels),
                   std::move(pixel_writer));
  PixelView view;
  PixelSolver solver(model, camera_metadata);

  PixelController controller({.camera_num = kStartCameraNum,
                              .max_camera_num = kNumCameras,
                              .model = model,
                              .view = view,
                              .solver = solver});

  constexpr char kWindowName[] = "window";
  cv::namedWindow(kWindowName, cv::WINDOW_KEEPRATIO);
  cv::setMouseCallback(
      kWindowName,
      [](int event, int x, int y, int flags, void* userdata) {
        PixelView* view = reinterpret_cast<PixelView*>(userdata);
        view->MouseEvent(event, {x, y});
      },
      &view);

  for (;;) {
    if (view.GetAndClearDirty()) {
      cv::imshow(kWindowName, view.Render());
    }
    if (int key = cv::waitKey(33); key != -1) {
      if (view.KeyboardEvent(key) == PixelView::KEYBOARD_QUIT) {
        break;
      }
    }
  }

  return 0;
}
