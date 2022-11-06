#include "cmd/showfound/model.h"

#include <functional>
#include <memory>

#include "absl/log/check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "cmd/showfound/camera_images.h"

PixelModel::PixelModel(std::vector<std::unique_ptr<CameraImages>> camera_images,
                       std::unique_ptr<std::vector<ModelPixel>> pixels,
                       std::unique_ptr<PixelWriter> pixel_writer)
    : camera_images_(std::move(camera_images)),
      pixels_(std::move(pixels)),
      pixel_writer_(std::move(pixel_writer)) {
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

cv::Mat PixelModel::GetAllOnImage(int camera_num) {
  QCHECK(IsValidCameraNum(camera_num)) << camera_num;
  return camera_images_[camera_num - 1]->on();
}

cv::Mat PixelModel::GetAllOffImage(int camera_num) {
  QCHECK(IsValidCameraNum(camera_num)) << camera_num;
  return camera_images_[camera_num - 1]->off();
}

absl::StatusOr<cv::Mat> PixelModel::GetPixelOnImage(int camera_num,
                                                    int pixel_num) {
  QCHECK(IsValidCameraNum(camera_num)) << camera_num;
  return camera_images_[camera_num - 1]->ReadImage(pixel_num);
}

bool PixelModel::IsValidCameraNum(int camera_num) {
  return camera_num > 0 && camera_num <= camera_images_.size();
}

absl::Status PixelModel::WritePixels() const {
  return pixel_writer_->WritePixels(*pixels_);
}

bool PixelModel::UpdatePixel(int pixel_num, const ModelPixel& pixel) {
  for (int i = 0; i < pixels_->size(); ++i) {
    const int num = (*pixels_)[i].num();

    if (num == pixel_num) {
      (*pixels_)[i] = pixel;
      return true;
    }
  }

  return false;
}
