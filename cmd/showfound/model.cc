#include "cmd/showfound/model.h"

Model::Model(absl::Span<const PixelState> pixels) {
  for (const auto& pixel : pixels) {
    pixels_.emplace(pixel.num, pixel);
  }
}

bool Model::ForEachPixel(
    std::function<bool(const PixelState& state)> callback) {
  for (const auto iter : pixels_) {
    if (!callback(iter.second)) {
      return false;
    }
  }
  return true;
}

Model::PixelState* const Model::FindPixel(int pixel_num) {
  if (auto iter = pixels_.find(pixel_num); iter != pixels_.end()) {
    return &iter->second;
  }
  return nullptr;
}
