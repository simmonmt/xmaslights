#ifndef _CMD_AUTOMAP_DETECTOR_H_
#define _CMD_AUTOMAP_DETECTOR_H_ 1

#include <vector>

#include "opencv2/opencv.hpp"

struct DetectResults {
  std::unordered_map<std::string, cv::Mat> intermediates;

  bool found;
  cv::Point centroid;
};

std::unique_ptr<DetectResults> Detect(cv::Mat off, cv::Mat on, cv::Mat mask);

#endif  // _CMD_AUTOMAP_DETECTOR_H_
