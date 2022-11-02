#include "cmd/showfound/model.h"

#include <functional>

#include "absl/log/check.h"
#include "absl/types/span.h"

PixelModel::PixelModel(cv::Mat ref_image, absl::Span<const PixelState> pixels)
    : ref_image_(ref_image) {
  for (auto& pixel : pixels) {
    pixels_.emplace(pixel.num, pixel);
  }
}

void PixelModel::ForEachPixel(
    int camera_num,
    std::function<void(const PixelState& state)> callback) const {
  QCHECK_EQ(1, camera_num);

  for (const auto iter : pixels_) {
    callback(iter.second);
  }
}

const PixelModel::PixelState* const PixelModel::FindPixel(int camera_num,
                                                          int pixel_num) const {
  QCHECK_EQ(1, camera_num);

  if (auto iter = pixels_.find(pixel_num); iter != pixels_.end()) {
    return &iter->second;
  }
  return nullptr;
}

cv::Mat PixelModel::GetRefImage(int camera_num) {
  QCHECK_EQ(1, camera_num);

  return ref_image_;
}
