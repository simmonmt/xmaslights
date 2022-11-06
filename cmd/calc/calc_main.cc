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

ABSL_FLAG(std::string, merged_coords, "",
          "File containing merged input coordinates. Each line is "
          "'pixelnum x,y x,y ...'. If no value is available, use - instead "
          "of x,y.");
ABSL_FLAG(double, camera_distance, -1,
          "Distance of the camera to the center of the object. Units are "
          "irrelevant.");
ABSL_FLAG(int, fov_h, -1, "Horizontal camera field of view, in degrees");
ABSL_FLAG(int, fov_v, -1, "Vertical camera field of view, in degrees");
ABSL_FLAG(int, resolution_h, -1, "Horizontal camera image resolution");
ABSL_FLAG(int, resolution_v, -1, "Vertical camera image resolution");
ABSL_FLAG(std::string, pcd_out, "", "PCD output file");
ABSL_FLAG(std::string, locations_out, "",
          "textproto output file with pixel locations");
ABSL_FLAG(bool, verbose, false, "Verbose mode");
ABSL_FLAG(int, camera_2_y_offset, 0,
          "Amount to add to y pixels from camera 2. Corrects for vertical "
          "misalignment.");

namespace {

proto::Point2i PointToProto(const cv::Point2i p) {
  proto::Point2i pr;
  pr.set_x(p.x);
  pr.set_y(p.y);
  return pr;
}

proto::Point3d PointToProto(const XYZPos& p) {
  proto::Point3d pr;
  pr.set_x(p.x);
  pr.set_y(p.y);
  pr.set_z(p.z);
  return pr;
}

absl::Status WriteLocations(const proto::PixelRecords& records,
                            const std::string& path) {
  return WriteTextProto(path, records);
}

absl::Status WritePCD(const std::vector<XYZPos>& points,
                      const std::string& path) {
  std::ofstream outfile(path);

  outfile << "VERSION .7\n";
  outfile << "FIELDS x y z rgb\n";
  outfile << "SIZE 4 4 4 4\n";
  outfile << "TYPE F F F U\n";
  outfile << "COUNT 1 1 1 1\n";
  outfile << "WIDTH " << points.size() << "\n";
  outfile << "HEIGHT 1\n";
  outfile << "VIEWPOINT 0 0 0 1 0 0 0\n";
  outfile << "POINTS " << points.size() << "\n";
  outfile << "DATA ascii\n";

  for (int i = 0; i < points.size(); ++i) {
    unsigned int color;
    if (i < 5) {
      color = 0x00ff00;
    } else if (i == points.size() - 1) {
      color = 0xff0000;
    } else {
      color = 0x0000ff;
    }

    const XYZPos& point = points[i];
    outfile << point.x << " " << point.y << " " << point.z << " " << color
            << "\n";
  }

  return absl::OkStatus();
}

}  // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("calculates 3d locations of pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  QCHECK_GT(absl::GetFlag(FLAGS_camera_distance), 0)
      << "--camera_distance is required";
  QCHECK_GT(absl::GetFlag(FLAGS_fov_h), 0) << "--fov_h is required";
  QCHECK_GT(absl::GetFlag(FLAGS_fov_v), 0) << "--fov_v is required";
  QCHECK_GT(absl::GetFlag(FLAGS_resolution_h), 0)
      << "--resolution_h is required";
  QCHECK_GT(absl::GetFlag(FLAGS_resolution_v), 0)
      << "--resolution_v is required";

  QCHECK(!absl::GetFlag(FLAGS_merged_coords).empty())
      << "--merged_coords is required";
  const std::vector<CoordsRecord> input = [&]() {
    auto status = ReadCoords(absl::GetFlag(FLAGS_merged_coords), std::nullopt);
    QCHECK_OK(status);
    return *status;
  }();

  const bool verbose = absl::GetFlag(FLAGS_verbose);

  const CameraMetadata metadata = {
      .distance_from_center = absl::GetFlag(FLAGS_camera_distance),
      .fov_h = Radians(absl::GetFlag(FLAGS_fov_h)),
      .fov_v = Radians(absl::GetFlag(FLAGS_fov_v)),
      .res_h = absl::GetFlag(FLAGS_resolution_h),
      .res_v = absl::GetFlag(FLAGS_resolution_v),
  };

  proto::PixelRecords out_pixels;

  std::vector<double> pixel_y_errors;
  std::vector<XYZPos> locations;
  int num_none = 0, num_one_only = 0, num_two_only = 0;
  for (const CoordsRecord& rec : input) {
    LOG_IF(INFO, verbose) << rec;

    proto::PixelRecord& out_pixel = *out_pixels.add_pixel();
    out_pixel.set_pixel_number(rec.pixel_num);
    for (int i = 0; i < rec.camera_coords.size(); ++i) {
      if (rec.camera_coords[i].has_value()) {
        const cv::Point2i detection = *rec.camera_coords[i];

        proto::CameraPixelLocation& out_camera_pixel =
            *out_pixel.add_camera_pixel();
        out_camera_pixel.set_camera_number(i + 1);
        *out_camera_pixel.mutable_pixel_location() = PointToProto(detection);
      }
    }

    QCHECK_EQ(rec.camera_coords.size(), 2);
    const std::optional<cv::Point2i>& detection1 = rec.camera_coords[0];
    const std::optional<cv::Point2i>& detection2 = rec.camera_coords[1];
    if (!detection1.has_value() || !detection2.has_value()) {
      LOG_IF(INFO, verbose)
          << "skipping pixel " << rec.pixel_num << "; need 2 detections";

      num_none += (!detection1.has_value() && !detection2.has_value());
      num_one_only += (detection1.has_value() && !detection2.has_value());
      num_two_only += (!detection1.has_value() && detection2.has_value());

      continue;
    }

    XYPos c1_pixel = {.x = static_cast<double>(detection1->x),
                      .y = static_cast<double>(detection1->y)};
    XYPos c2_pixel = {.x = static_cast<double>(detection2->x),
                      .y = static_cast<double>(detection2->y)};
    c2_pixel.y +=
        std::min(absl::GetFlag(FLAGS_camera_2_y_offset), metadata.res_v);

    Result result = FindDetectionLocation(c1_pixel, c2_pixel, metadata);
    pixel_y_errors.push_back(result.pixel_y_error);
    locations.push_back(result.detection);

    *out_pixel.mutable_world_pixel()->mutable_pixel_location() =
        PointToProto(result.detection);

    if (absl::GetFlag(FLAGS_verbose)) {
      std::cout << absl::StreamFormat("%03d %d\n", rec.pixel_num,
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

  std::cerr << absl::StrFormat(
      "in: %d, located: %d (none: %d, one: %d, two: %d), yErr avg: %f, median: "
      "%f\n",
      input.size(), num, num_none, num_one_only, num_two_only, avg, median);

  if (!absl::GetFlag(FLAGS_locations_out).empty()) {
    QCHECK_OK(WriteLocations(out_pixels, absl::GetFlag(FLAGS_locations_out)));
  }

  if (!absl::GetFlag(FLAGS_pcd_out).empty()) {
    QCHECK_OK(WritePCD(locations, absl::GetFlag(FLAGS_pcd_out)));
  }

  return 0;
}
