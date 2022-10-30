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
#include "cmd/showfound/ui.h"
#include "lib/file/coords.h"
#include "lib/file/readers.h"
#include "opencv2/opencv.hpp"

ABSL_FLAG(int, camera_num, -1, "camera number");
ABSL_FLAG(std::string, ref_image, "", "Path to reference image");
ABSL_FLAG(std::string, merged_coords, "",
          "File containing merged input coordinates. Each line is "
          "'pixelnum x,y x,y ...'. If no value is available, use - instead "
          "of x,y.");
ABSL_FLAG(std::string, final_coords, "",
          "Path to file containing list of final coordinates");
ABSL_FLAG(bool, verbose, false, "Verbose mode");

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("show mapped pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  const bool verbose = absl::GetFlag(FLAGS_verbose);

  QCHECK(absl::GetFlag(FLAGS_camera_num) > 0)
      << "--ref_image is required; must be positive";

  QCHECK(!absl::GetFlag(FLAGS_ref_image).empty()) << "--ref_image is required";
  cv::Mat image = cv::imread(absl::GetFlag(FLAGS_ref_image));

  QCHECK(!absl::GetFlag(FLAGS_merged_coords).empty())
      << "--merged_coords is required";
  QCHECK(!absl::GetFlag(FLAGS_final_coords).empty())
      << "--final_coords is required";
  const std::vector<CoordsRecord> input = [&]() {
    auto status = ReadCoords(absl::GetFlag(FLAGS_merged_coords),
                             absl::GetFlag(FLAGS_final_coords));
    QCHECK_OK(status);
    return *status;
  }();

  auto skip_message = [](int num, const std::string& suffix) {
    return absl::StrFormat("skipping pixel %d: %s", num, suffix);
  };

  std::vector<PixelState> pixels;
  for (const CoordsRecord& rec : input) {
    LOG_IF(INFO, verbose) << rec;

    const int coord_idx = absl::GetFlag(FLAGS_camera_num) - 1;
    QCHECK_LT(coord_idx, rec.camera_coords.size()) << "camera num too big";
    if (!rec.camera_coords[coord_idx].has_value()) {
      LOG_IF(INFO, verbose)
          << skip_message(rec.pixel_num, "no coords for camera");
      continue;
    }
    PixelState::Knowledge knowledge = rec.final_coord.has_value()
                                          ? PixelState::CALCULATED
                                          : PixelState::THIS_ONLY;

    PixelState pixel = {
        .num = rec.pixel_num,
        .coords = *rec.camera_coords[coord_idx],
        .calc = rec.final_coord.value_or(cv::Point3d()),
        .knowledge = knowledge,
    };
    pixels.push_back(pixel);
  }

  PixelUI ui(image, pixels);

  constexpr char kWindowName[] = "window";
  cv::namedWindow(kWindowName, cv::WINDOW_KEEPRATIO);
  cv::setMouseCallback(
      kWindowName,
      [](int event, int x, int y, int flags, void* userdata) {
        PixelUI* ui = reinterpret_cast<PixelUI*>(userdata);
        ui->MouseEvent(event, {x, y});
      },
      &ui);

  for (;;) {
    if (ui.GetAndClearDirty()) {
      cv::imshow(kWindowName, ui.Render());
    }
    if (int key = cv::waitKey(33); key != -1) {
      if (ui.KeyboardEvent(key) == PixelUI::KEYBOARD_QUIT) {
        break;
      }
    }
  }

  return 0;
}
