#ifndef _LIB_FILE_COORDS_H_
#define _LIB_FILE_COORDS_H_ 1

#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "absl/status/statusor.h"
#include "opencv2/core/types.hpp"

struct CoordsRecord {
  int pixel_num;
  std::vector<std::optional<cv::Point2i>> camera_coords;
  std::optional<cv::Point3d> final_coord;
};

std::ostream& operator<<(std::ostream& os, const CoordsRecord& rec);

absl::StatusOr<std::vector<CoordsRecord>> ReadCoords(
    std::optional<std::string> merged_coords_path,
    std::optional<std::string> final_coords_path);

#endif  // _LIB_FILE_COORDS_H_
