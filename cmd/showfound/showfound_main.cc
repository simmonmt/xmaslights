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

ABSL_FLAG(std::string, ref_image, "", "Path to reference image");
ABSL_FLAG(std::string, merged_coords, "",
          "File containing merged input coordinates. Each line is "
          "'pixelnum x,y x,y ...'. If no value is available, use - instead "
          "of x,y.");
ABSL_FLAG(std::string, world_coords, "",
          "Path to file containing list of world coordinates");
ABSL_FLAG(bool, verbose, false, "Verbose mode");

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("show mapped pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  const bool verbose = absl::GetFlag(FLAGS_verbose);

  QCHECK(!absl::GetFlag(FLAGS_ref_image).empty()) << "--ref_image is required";
  cv::Mat image = cv::imread(absl::GetFlag(FLAGS_ref_image));

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

  auto skip_message = [](int num, const std::string& suffix) {
    return absl::StrFormat("skipping pixel %d: %s", num, suffix);
  };

  constexpr int kStartCameraNum = 1;

  std::vector<PixelModel::PixelState> pixels;
  for (const CoordsRecord& rec : input) {
    LOG_IF(INFO, verbose) << rec;

    const int coord_idx = kStartCameraNum - 1;
    QCHECK_LT(coord_idx, rec.camera_coords.size()) << "camera num too big";
    if (!rec.camera_coords[coord_idx].has_value()) {
      LOG_IF(INFO, verbose)
          << skip_message(rec.pixel_num, "no coords for camera");
      continue;
    }

    PixelModel::PixelState pixel = {
        .num = rec.pixel_num,
        .camera = *rec.camera_coords[coord_idx],
        .world = rec.world_coord,
        .synthesized = false,
    };
    pixels.push_back(pixel);
  }

  PixelModel model(image, pixels);
  PixelView view;
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
