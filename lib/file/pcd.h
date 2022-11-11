#ifndef _LIB_FILE_PCD_H_
#define _LIB_FILE_PCD_H_ 1

#include <string>
#include <tuple>
#include <vector>

#include "absl/status/status.h"
#include "opencv2/core/types.hpp"
#include "opencv2/viz/types.hpp"

absl::Status WritePCD(
    const std::vector<std::tuple<int, cv::Point3d, cv::viz::Color>>& points,
    const std::string& path);

#endif  // _LIB_FILE_PCD_H_
