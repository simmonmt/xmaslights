#include "cmd/showfound/model.h"

Model::Model(absl::Span<const PixelState> pixels) {
  for (auto& pixel : pixels) {
    pixels_.emplace(pixel.num, pixel);
  }
}

void Model::ForEachPixel(
    std::function<void(const PixelState& state)> callback) {
  for (const auto iter : pixels_) {
    callback(iter.second);
  }
}

Model::PixelState* const Model::FindPixel(int pixel_num) {
  if (auto iter = pixels_.find(pixel_num); iter != pixels_.end()) {
    return &iter->second;
  }
  return nullptr;
}
