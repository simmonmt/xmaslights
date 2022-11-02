#include "cmd/showfound/model.h"

#include <functional>

#include "absl/log/check.h"
#include "absl/types/span.h"

PixelModel::PixelModel(std::unique_ptr<std::vector<cv::Mat>> ref_images,
                       std::unique_ptr<std::vector<ModelPixel>> pixels)
    : ref_images_(std::move(ref_images)), pixels_(std::move(pixels)) {
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
  QCHECK(camera_num > 0 && camera_num <= ref_images_->size()) << camera_num;
  return (*ref_images_)[camera_num - 1];
}
