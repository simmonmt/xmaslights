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
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

class ModelPixel {
 public:
  ModelPixel(int num, std::vector<std::optional<cv::Point2i>> cameras,
             std::optional<cv::Point3d> world, bool synthesized = false)
      : num_(num),
        cameras_(cameras),
        world_(world),
        synthesized_(synthesized) {}

  int num() const { return num_; }

  bool has_camera(int camera_num) const {
    return cameras_[camera_num - 1].has_value();
  }
  cv::Point2i camera(int camera_num) const {
    QCHECK(has_camera(camera_num)) << camera_num;
    return *cameras_[camera_num - 1];
  }

  bool has_world() const { return world_.has_value(); }
  cv::Point3d world() const { return *world_; }

  bool synthesized() const { return synthesized_; }

 private:
  int num_;
  std::vector<std::optional<cv::Point2i>> cameras_;
  std::optional<cv::Point3d> world_;

  bool synthesized_;
};

class PixelModel {
 public:
  PixelModel(std::vector<std::unique_ptr<CameraImages>> camera_images,
             std::unique_ptr<std::vector<ModelPixel>> pixels);
  ~PixelModel() = default;

  cv::Mat GetRefImage(int camera_num);

  void ForEachPixel(
      std::function<void(const ModelPixel& pixel)> callback) const;
  const ModelPixel* const FindPixel(int pixel_num) const;

 private:
  std::vector<std::unique_ptr<CameraImages>> camera_images_;
  std::unique_ptr<std::vector<ModelPixel>> pixels_;
  std::unordered_map<int, ModelPixel*> pixels_by_num_;
};

#endif  // _CMD_SHOWFOUND_MODEL_H_
