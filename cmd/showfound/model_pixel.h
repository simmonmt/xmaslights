#ifndef _CMD_SHOWFOUND_MODEL_PIXEL_H_
#define _CMD_SHOWFOUND_MODEL_PIXEL_H_ 1

#include <optional>
#include <vector>

#include "absl/log/check.h"
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

  bool has_any_camera() const {
    for (const auto& camera : cameras_) {
      if (camera.has_value()) {
        return true;
      }
    }
    return false;
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

#endif  // _CMD_SHOWFOUND_MODEL_PIXEL_H_
