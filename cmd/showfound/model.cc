#include "cmd/showfound/model.h"

PixelModel::PixelModel(absl::Span<const PixelState> pixels) {
  for (auto& pixel : pixels) {
    pixels_.emplace(pixel.num, pixel);
  }
}

void PixelModel::ForEachPixel(
    std::function<void(const PixelState& state)> callback) const {
  for (const auto iter : pixels_) {
    callback(iter.second);
  }
}

const PixelModel::PixelState* const PixelModel::FindPixel(int pixel_num) const {
  if (auto iter = pixels_.find(pixel_num); iter != pixels_.end()) {
    return &iter->second;
  }
  return nullptr;
}
