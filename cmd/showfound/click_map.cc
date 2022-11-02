#include "cmd/showfound/click_map.h"

#include <tuple>

#include "absl/types/span.h"
#include "opencv2/opencv.hpp"

namespace {

constexpr int kBufferSize = 5;

}  // namespace

ClickMap::ClickMap(cv::Size size,
                   absl::Span<const std::tuple<int, cv::Point2i>> targets) {
  map_ = cv::Mat(size, CV_32S, -1);

  std::for_each(targets.begin(), targets.end(),
                [&](const std::tuple<int, cv::Point2i>& target) {
                  const auto& [num, coord] = target;

                  cv::rectangle(
                      map_,
                      {std::max(coord.x - kBufferSize, 0),
                       std::max(coord.y - kBufferSize, 0)},
                      {std::min(coord.x + kBufferSize, size.width - 1),
                       std::min(coord.y + kBufferSize, size.height - 1)},
                      num, -1);
                });
}

int ClickMap::WhichTarget(cv::Point2i point) { return map_.at<int>(point); }
