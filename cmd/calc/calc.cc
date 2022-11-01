#include "cmd/calc/calc.h"

#include <math.h>

#include "absl/log/log.h"
#include "lib/geometry/camera_metadata.h"
#include "lib/geometry/points.h"

namespace {

constexpr double PI_3 = M_PI / 3.0;

}  // namespace

std::ostream& operator<<(std::ostream& os, const Line& pos) {
  return os << "y=" << pos.slope << "x+" << pos.b;
}

double Degrees(double rad) { return (rad / M_PI) * 180; }

double Radians(double deg) { return (deg / 180) * M_PI; }

XYPos FindC1XYPos(double D) { return {.x = D, .y = 0}; }

XYPos FindC2XYPos(double D) {
  return {.x = -std::cos(PI_3) * D, .y = std::sin(PI_3) * D};
}

double FindAngleRad(int pixel, int res, double fov) {
  double res_half = res / 2.0;
  return (fov / 2.0) * (static_cast<double>(pixel - res_half) / res_half);
}

double FindDist(const XYPos& p1, const XYPos& p2) {
  double dx = p1.x - p2.x;
  double dy = p1.y - p2.y;
  return std::sqrt(dx * dx + dy * dy);
}

double FindDetectionZ(double z_angle, const XYPos& camera,
                      const XYPos& detection) {
  return std::tan(z_angle) * FindDist(camera, detection);
}

double FindLineB(double slope, const XYPos& pos) {
  return pos.y - slope * pos.x;
}

XYPos FindXYIntersection(const Line& l1, const Line& l2) {
  double dx = (l2.b - l1.b) / (l1.slope - l2.slope);
  double dy = l1.slope * dx + l1.b;
  return {.x = dx, .y = dy};
}

Result FindDetectionLocation(const XYPos& c1_pixel, const XYPos& c2_pixel,
                             const CameraMetadata& metadata) {
  XYPos c1 = FindC1XYPos(metadata.distance_from_center);
  XYPos c2 = FindC2XYPos(metadata.distance_from_center);

  double c1_xyangle =
      M_PI - FindAngleRad(c1_pixel.x, metadata.res_h, metadata.fov_h);
  double c2_xyangle =
      2.0 * PI_3 - FindAngleRad(c2_pixel.x, metadata.res_h, metadata.fov_h);

  Line c1_line;
  c1_line.slope = std::tan(c1_xyangle);
  c1_line.b = FindLineB(c1_line.slope, c1);

  Line c2_line;
  c2_line.slope = std::tan(c2_xyangle);
  c2_line.b = FindLineB(c2_line.slope, c2);

  XYPos det_xy = FindXYIntersection(c1_line, c2_line);

  double c1_zangle = -FindAngleRad(c1_pixel.y, metadata.res_v, metadata.fov_v);
  double c2_zangle = -FindAngleRad(c2_pixel.y, metadata.res_v, metadata.fov_v);

  double c1_det_z = FindDetectionZ(c1_zangle, c1, det_xy);
  double c2_det_z = FindDetectionZ(c2_zangle, c2, det_xy);

  double det_z = (c1_det_z + c2_det_z) / 2.0;

  return {
      .detection = {.x = det_xy.x, .y = det_xy.y, .z = det_z},
      .pixel_y_error = c2_pixel.y - c1_pixel.y,
  };
}
