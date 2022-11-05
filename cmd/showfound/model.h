#ifndef _CMD_SHOWFOUND_MODEL_H_
#define _CMD_SHOWFOUND_MODEL_H_ 1

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "absl/log/check.h"
#include "absl/types/span.h"
#include "cmd/showfound/camera_images.h"
#include "cmd/showfound/model_pixel.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

class PixelModel {
 public:
  PixelModel(std::vector<std::unique_ptr<CameraImages>> camera_images,
             std::unique_ptr<std::vector<ModelPixel>> pixels);
  ~PixelModel() = default;

  bool IsValidCameraNum(int camera_num);

  cv::Mat GetAllOnImage(int camera_num);
  cv::Mat GetAllOffImage(int camera_num);
  absl::StatusOr<cv::Mat> GetPixelOnImage(int camera_num, int pixel_num);

  void ForEachPixel(
      std::function<void(const ModelPixel& pixel)> callback) const;
  const ModelPixel* const FindPixel(int pixel_num) const;

 private:
  std::vector<std::unique_ptr<CameraImages>> camera_images_;
  std::unique_ptr<std::vector<ModelPixel>> pixels_;
  std::unordered_map<int, ModelPixel*> pixels_by_num_;
};

#endif  // _CMD_SHOWFOUND_MODEL_H_
