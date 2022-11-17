#ifndef _CMD_SHOWFOUND_MODEL_PIXEL_H_
#define _CMD_SHOWFOUND_MODEL_PIXEL_H_ 1

#include <optional>
#include <vector>

#include "absl/log/check.h"
#include "opencv2/core/types.hpp"
#include "proto/points.pb.h"

class ModelPixel {
 public:
  ModelPixel(int num, std::vector<std::optional<cv::Point2i>> cameras,
             std::optional<cv::Point3d> world);

  ModelPixel(const proto::PixelRecord& pixel);

  const proto::PixelRecord& ToProto() const { return pixel_; }

  int num() const { return pixel_.pixel_number(); }

  bool has_camera(int camera_num) const {
    for (const auto& camera : pixel_.camera_pixel()) {
      if (camera.camera_number() == camera_num) {
        return true;
      }
    }
    return false;
  }

  std::vector<int> cameras() const {
    std::vector<int> out;
    for (const auto& camera : pixel_.camera_pixel()) {
      out.push_back(camera.camera_number());
    }
    return out;
  }

  bool has_other_camera(int camera_num) const {
    for (const auto& camera : pixel_.camera_pixel()) {
      if (camera.camera_number() != camera_num) {
        return true;
      }
    }
    return false;
  }

  bool has_any_camera() const { return pixel_.camera_pixel_size() != 0; }

  cv::Point2i camera(int camera_num) const {
    for (const auto& camera : pixel_.camera_pixel()) {
      if (camera.camera_number() == camera_num) {
        return cv::Point2i(camera.pixel_location().x(),
                           camera.pixel_location().y());
      }
    }
    CHECK(false) << "invalid camera number " << camera_num << " for pixel "
                 << num();
  }

  bool has_world() const { return pixel_.has_world_pixel(); }
  cv::Point3d world() const {
    const proto::Point3d& loc = pixel_.world_pixel().pixel_location();
    return cv::Point3d(loc.x(), loc.y(), loc.z());
  }

 private:
  proto::PixelRecord pixel_;
};

class ModelPixelBuilder {
 public:
  ModelPixelBuilder(const ModelPixel& orig);

  ModelPixelBuilder& SetCameraLocation(int camera_num, cv::Point2i location,
                                       bool manual_update);
  ModelPixelBuilder& SetWorldLocation(cv::Point3d location);

  ModelPixel Build();

 private:
  proto::PixelRecord proto_;
};

#endif  // _CMD_SHOWFOUND_MODEL_PIXEL_H_
