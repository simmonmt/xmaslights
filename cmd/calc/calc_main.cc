#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "cmd/calc/calc.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "lib/file/coords.h"
#include "lib/file/proto.h"
#include "lib/geometry/translation.h"
#include "proto/points.pb.h"

ABSL_FLAG(std::string, input_coords, "",
          "File containing coordinates in proto.PixelRecords textproto format");
ABSL_FLAG(std::string, output_coords, "",
          "File containing coordinates in proto.PixelRecords textproto format");
ABSL_FLAG(std::string, camera_metadata, "",
          "File containing CameraMetadata textproto");
ABSL_FLAG(bool, verbose, false, "Verbose mode");
ABSL_FLAG(int, camera_2_y_offset, 0,
          "Amount to add to y pixels from camera 2. Corrects for vertical "
          "misalignment.");

namespace {

proto::Point3d PointToProto(const XYZPos& p) {
  proto::Point3d pr;
  pr.set_x(p.x);
  pr.set_y(p.y);
  pr.set_z(p.z);
  return pr;
}

absl::StatusOr<std::unique_ptr<proto::PixelRecords>> ReadPixelsFromProto(
    const std::string& path) {
  auto records = std::make_unique<proto::PixelRecords>();
  if (absl::Status status = ReadProto(path, records.get()); !status.ok()) {
    return status;
  }
  return records;
}

}  // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("calculates 3d locations of pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  QCHECK(!absl::GetFlag(FLAGS_input_coords).empty())
      << "--input_coords is required";
  std::unique_ptr<proto::PixelRecords> pixels = [&]() {
    auto status = ReadPixelsFromProto(absl::GetFlag(FLAGS_input_coords));
    QCHECK_OK(status);
    return std::move(*status);
  }();

  const bool verbose = absl::GetFlag(FLAGS_verbose);

  QCHECK(!absl::GetFlag(FLAGS_camera_metadata).empty())
      << "--camera_metadata is required";
  const CameraMetadata camera_metadata = [&]() {
    auto result =
        ReadProto<proto::CameraMetadata>(absl::GetFlag(FLAGS_camera_metadata));
    QCHECK_OK(result);
    return CameraMetadata::FromProto(*result);
  }();

  std::vector<double> pixel_y_errors;
  for (proto::PixelRecord& rec : *pixels->mutable_pixel()) {
    LOG_IF(INFO, verbose) << rec.ShortDebugString();
    std::optional<cv::Point2i> detection1;
    std::optional<cv::Point2i> detection2;
    for (const proto::CameraPixelLocation& camera : rec.camera_pixel()) {
      cv::Point2i point(camera.pixel_location().x(),
                        camera.pixel_location().y());
      if (camera.camera_number() == 1) {
        detection1 = point;
      } else {
        QCHECK_EQ(camera.camera_number(), 2) << "unexpected camera number";
        detection2 = point;
      }
    }

    if (!detection1.has_value() || !detection2.has_value()) {
      LOG(INFO) << "skipping pixel " << rec.pixel_number()
                << "; need 2 cameras";
      continue;
    }

    XYPos c1_pixel = {.x = static_cast<double>(detection1->x),
                      .y = static_cast<double>(detection1->y)};
    XYPos c2_pixel = {.x = static_cast<double>(detection2->x),
                      .y = static_cast<double>(detection2->y)};
    c2_pixel.y +=
        std::min(absl::GetFlag(FLAGS_camera_2_y_offset), camera_metadata.res_v);

    Result result = FindDetectionLocation(c1_pixel, c2_pixel, camera_metadata);
    pixel_y_errors.push_back(result.pixel_y_error);

    *rec.mutable_world_pixel()->mutable_pixel_location() =
        PointToProto(result.detection);

    if (absl::GetFlag(FLAGS_verbose)) {
      std::cout << absl::StreamFormat("%03d %d\n", rec.pixel_number(),
                                      result.detection);
    }
  }

  double avg = 0;
  int num = 0;
  for (double err : pixel_y_errors) {
    avg += err;
    ++num;
  }
  avg /= num;

  double median = 0;
  if (!pixel_y_errors.empty()) {
    int middle = pixel_y_errors.size() / 2;
    if (pixel_y_errors.size() % 2 == 1) {
      median = pixel_y_errors[middle] + 1;
    } else {
      median = (pixel_y_errors[middle] + pixel_y_errors[middle + 1]) / 2;
    }
  }

  std::cerr << absl::StrFormat("yErr avg: %f, median: %f\n", avg, median);

  if (const std::string& path = absl::GetFlag(FLAGS_output_coords);
      !path.empty()) {
    QCHECK_OK(WriteTextProto(path, *pixels));
  }

  return 0;
}
