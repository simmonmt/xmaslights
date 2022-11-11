#include "lib/file/pcd.h"

#include <fstream>

#include "absl/strings/str_format.h"
#include "lib/cv/cv.h"
#include "opencv2/viz/types.hpp"

absl::Status WritePCD(
    const std::vector<std::tuple<int, cv::Point3d, cv::viz::Color>>& points,
    const std::string& path) {
  std::ofstream out;
  out.open(path);
  if (!out.good()) {
    return absl::UnknownError("failed to open file for writing");
  }

  out << "VERSION .7\n";
  out << "FIELDS x y z rgb label\n";
  out << "SIZE 4 4 4 4 4\n";
  out << "TYPE F F F U U\n";
  out << "COUNT 1 1 1 1 1\n";
  out << "WIDTH " << points.size() << "\n";
  out << "HEIGHT 1\n";
  out << "VIEWPOINT 0 0 0 1 0 0 0\n";
  out << "POINTS " << points.size() << "\n";
  out << "DATA ascii\n";

  for (const auto& [num, point, color] : points) {
    out << absl::StreamFormat("%f %f %f 0x%06x %d\n", point.x, point.y, point.z,
                              CvColorToRgbBytes(color), num);
  }

  out.close();
  if (!out.good()) {
    return absl::UnknownError("failed to close file");
  }

  return absl::OkStatus();
}
