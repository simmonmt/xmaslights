#include <fstream>
#include <iostream>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "lib/file/proto.h"
#include "lib/file/readers.h"
#include "opencv2/opencv.hpp"
#include "proto/points.pb.h"

ABSL_FLAG(std::string, input_coords, "",
          "File containing coordinates in proto.PixelRecords textproto format");

namespace {

absl::StatusOr<std::unique_ptr<std::vector<proto::PixelRecord>>>
ReadPixelsFromProto(const std::string& path) {
  proto::PixelRecords records;
  if (absl::Status status = ReadProto(path, &records); !status.ok()) {
    return status;
  }

  auto pixels = std::make_unique<std::vector<proto::PixelRecord>>();
  std::copy(records.pixel().begin(), records.pixel().end(),
            std::back_inserter(*pixels));

  return pixels;
}

}  // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("show mapped pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  QCHECK(!absl::GetFlag(FLAGS_input_coords).empty())
      << "--input_coords is required";

  std::unique_ptr<std::vector<proto::PixelRecord>> pixels = [&] {
    auto status = ReadPixelsFromProto(absl::GetFlag(FLAGS_input_coords));
    QCHECK_OK(status);
    return std::move(*status);
  }();

  constexpr char kWindowName[] = "window";
  cv::namedWindow(kWindowName, cv::WINDOW_NORMAL | cv::WINDOW_FREERATIO);
  cv::setMouseCallback(
      kWindowName, [](int event, int x, int y, int flags, void* userdata) {},
      nullptr);

  return 0;
}
