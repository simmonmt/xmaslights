#ifndef _CMD_SHOWFOUND_CLICK_MAP_H_
#define _CMD_SHOWFOUND_CLICK_MAP_H_ 1

#include "absl/types/span.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <tuple>

class ClickMap {
 public:
  ClickMap(cv::Size size,
           absl::Span<const std::tuple<int, cv::Point2i>> targets);
  ~ClickMap() = default;

  int WhichTarget(cv::Point2i point);

 private:
  cv::Mat map_;
};

#endif  // _CMD_SHOWFOUND_CLICK_MAP_H_
