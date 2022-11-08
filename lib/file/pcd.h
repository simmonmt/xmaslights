#ifndef _LIB_FILE_PCD_H_
#define _LIB_FILE_PCD_H_ 1

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "opencv2/core/types.hpp"

absl::Status WritePCD(const std::vector<cv::Point3d>& points,
                      const std::string& path);

#endif  // _LIB_FILE_PCD_H_
