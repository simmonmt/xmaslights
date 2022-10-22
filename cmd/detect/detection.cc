#include "cmd/detect/detection.h"

#include "absl/log/log.h"
#include "opencv2/opencv.hpp"
#include "opencv2/viz/types.hpp"

// This isn't great. It's certainly nowhere near as fancy as it could
// get. But it works with images gathered in pitch darkness, which is
// easy enough to do.

std::unique_ptr<DetectResults> Detect(cv::Mat off, cv::Mat on, cv::Mat mask) {
  cv::Mat masked_off, masked_on;
  cv::bitwise_and(off, off, masked_off, mask);
  cv::bitwise_and(on, on, masked_on, mask);

  cv::Mat gray_on, gray_off;
  cv::cvtColor(masked_off, gray_off, cv::COLOR_BGR2GRAY);
  cv::cvtColor(masked_on, gray_on, cv::COLOR_BGR2GRAY);

  cv::Mat absdiff, threshold, eroded;
  cv::absdiff(gray_off, gray_on, absdiff);
  cv::threshold(absdiff, threshold, 80, 255, cv::THRESH_BINARY);
  cv::erode(threshold, eroded,
            cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

  auto results = std::make_unique<DetectResults>();
  results->intermediates["gray_off"] = gray_off;
  results->intermediates["gray_on"] = gray_on;
  results->intermediates["absdiff"] = absdiff;
  results->intermediates["threshold"] = threshold;
  results->intermediates["eroded"] = eroded;

  std::vector<std::vector<cv::Point>> found_contours;
  cv::findContours(eroded, found_contours, cv::RETR_TREE,
                   cv::CHAIN_APPROX_SIMPLE);

  if (found_contours.size() > 0) {
    int max_idx = -1;
    double max_area;
    for (unsigned int i = 0; i < found_contours.size(); ++i) {
      const std::vector<cv::Point>& contour = found_contours[i];
      double area = cv::contourArea(contour);
      if (max_idx == -1 || area > max_area) {
        max_idx = i;
        max_area = area;
      }
    }

    const std::vector<cv::Point>& biggest_contour = found_contours[max_idx];

    cv::Moments moments = cv::moments(biggest_contour);
    results->centroid.x = int(moments.m10 / moments.m00);
    results->centroid.y = int(moments.m01 / moments.m00);

    results->found = true;

    cv::Mat marked = on.clone();
    cv::drawMarker(marked, results->centroid, cv::viz::Color::red(),
                   cv::MARKER_CROSS, 50, 2);
    results->intermediates["marked"] = marked;
  }

  return results;
}
