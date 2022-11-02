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
#include "cmd/showfound/controller.h"
#include "cmd/showfound/model.h"
#include "cmd/showfound/view.h"
#include "lib/file/coords.h"
#include "lib/file/readers.h"
#include "opencv2/opencv.hpp"

ABSL_FLAG(std::string, ref_images, "",
          "Comma-separated paths to reference images");
ABSL_FLAG(std::string, merged_coords, "",
          "File containing merged input coordinates. Each line is "
          "'pixelnum x,y x,y ...'. If no value is available, use - instead "
          "of x,y.");
ABSL_FLAG(std::string, world_coords, "",
          "Path to file containing list of world coordinates");
ABSL_FLAG(bool, verbose, false, "Verbose mode");

namespace {

std::unique_ptr<std::vector<cv::Mat>> ReadRefImages(const std::string& paths) {
  auto images = std::make_unique<std::vector<cv::Mat>>();
  std::vector<std::string> parts = absl::StrSplit(paths, ",");
  for (const std::string& path : parts) {
    images->push_back(cv::imread(path));
  }
  return images;
}

}  // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("show mapped pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  constexpr int kNumCameras = 2;
  constexpr int kStartCameraNum = 1;

  const bool verbose = absl::GetFlag(FLAGS_verbose);

  QCHECK(!absl::GetFlag(FLAGS_ref_images).empty())
      << "--ref_images is required";
  std::unique_ptr<std::vector<cv::Mat>> ref_images =
      ReadRefImages(absl::GetFlag(FLAGS_ref_images));
  QCHECK_EQ(ref_images->size(), kNumCameras)
      << "num cameras ref images mismatch";

  QCHECK(!absl::GetFlag(FLAGS_merged_coords).empty())
      << "--merged_coords is required";
  QCHECK(!absl::GetFlag(FLAGS_world_coords).empty())
      << "--world_coords is required";
  const std::vector<CoordsRecord> input = [&]() {
    auto status = ReadCoords(absl::GetFlag(FLAGS_merged_coords),
                             absl::GetFlag(FLAGS_world_coords));
    QCHECK_OK(status);
    return *status;
  }();

  auto pixels = std::make_unique<std::vector<ModelPixel>>();
  for (const CoordsRecord& rec : input) {
    LOG_IF(INFO, verbose) << rec;

    QCHECK_EQ(rec.camera_coords.size(), kNumCameras)
        << "num cameras camera coords mismatch";
    ModelPixel pixel(rec.pixel_num, rec.camera_coords, rec.world_coord);
    pixels->push_back(pixel);
  }

  PixelModel model(std::move(ref_images), std::move(pixels));
  PixelView view(kNumCameras);
  PixelController controller(kStartCameraNum, model, view);

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
