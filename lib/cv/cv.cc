#include "lib/cv/cv.h"

#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "opencv2/highgui.hpp"
#include "opencv2/opencv.hpp"

absl::StatusOr<cv::Mat> CvReadImage(const std::string& path) {
  cv::Mat img = cv::imread(path);
  if (img.empty()) {
    return absl::InternalError(
        absl::StrFormat("failed to read image from %s", path));
  }
  return img;
}
