#ifndef _LIB_CV_CV_H_
#define _LIB_CV_CV_H_ 1

#include <string>

#include "absl/status/statusor.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/viz/types.hpp"

absl::StatusOr<cv::Mat> CvReadImage(const std::string& path);

unsigned long CvColorToRgbBytes(cv::viz::Color color);

#endif  // _LIB_CV_CV_H_
