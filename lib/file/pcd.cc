#include "lib/file/pcd.h"

#include <fstream>

absl::Status WritePCD(const std::vector<cv::Point3d>& points,
                      const std::string& path) {
  std::ofstream out;
  out.open(path);
  if (!out.good()) {
    return absl::UnknownError("failed to open file for writing");
  }

  out << "VERSION .7\n";
  out << "FIELDS x y z rgb\n";
  out << "SIZE 4 4 4 4\n";
  out << "TYPE F F F U\n";
  out << "COUNT 1 1 1 1\n";
  out << "WIDTH " << points.size() << "\n";
  out << "HEIGHT 1\n";
  out << "VIEWPOINT 0 0 0 1 0 0 0\n";
  out << "POINTS " << points.size() << "\n";
  out << "DATA ascii\n";

  for (int i = 0; i < points.size(); ++i) {
    unsigned int color;
    if (i < 5) {
      color = 0x00ff00;
    } else if (i == points.size() - 1) {
      color = 0xff0000;
    } else {
      color = 0x0000ff;
    }

    const cv::Point3d& point = points[i];
    out << point.x << " " << point.y << " " << point.z << " " << color << "\n";
  }

  out.close();
  if (!out.good()) {
    return absl::UnknownError("failed to close file");
  }

  return absl::OkStatus();
}
