#include "cmd/showfound/model.h"

#include <functional>

#include "absl/log/check.h"
#include "absl/types/span.h"

PixelModel::PixelModel(cv::Mat ref_image,
                       std::unique_ptr<std::vector<ModelPixel>> pixels)
    : ref_image_(ref_image), pixels_(std::move(pixels)) {
  for (auto& pixel : *pixels_) {
    pixels_by_num_.emplace(pixel.num(), &pixel);
  }
}

void PixelModel::ForEachPixel(
    std::function<void(const ModelPixel& pixel)> callback) const {
  for (const ModelPixel& pixel : *pixels_) {
    callback(pixel);
  }
}

const ModelPixel* const PixelModel::FindPixel(int pixel_num) const {
  if (auto iter = pixels_by_num_.find(pixel_num);
      iter != pixels_by_num_.end()) {
    return iter->second;
  }
  return nullptr;
}

cv::Mat PixelModel::GetRefImage(int camera_num) {
  QCHECK_EQ(1, camera_num);

  return ref_image_;
}
