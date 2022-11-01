#ifndef _CMD_SHOWFOUND_MODEL_H_
#define _CMD_SHOWFOUND_MODEL_H_ 1

#include <functional>
#include <optional>
#include <unordered_map>

#include "absl/types/span.h"
#include "opencv2/core/types.hpp"

class PixelModel {
 public:
  struct PixelState {
    int num;
    cv::Point2i camera;
    std::optional<cv::Point3d> world;

    bool synthesized;
  };

  PixelModel(absl::Span<const PixelState> pixels);
  ~PixelModel() = default;

  void ForEachPixel(
      std::function<void(const PixelState& state)> callback) const;
  const PixelState* const FindPixel(int pixel_num) const;

 private:
  std::unordered_map<int, PixelState> pixels_;
};

#endif  // _CMD_SHOWFOUND_MODEL_H_
