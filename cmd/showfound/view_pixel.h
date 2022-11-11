#ifndef _CMD_SHOWFOUND_VIEW_PIXEL_H_
#define _CMD_SHOWFOUND_VIEW_PIXEL_H_ 1

#include <optional>

#include "absl/log/check.h"
#include "opencv2/core/types.hpp"

class ViewPixel {
 public:
  enum Knowledge {
    CALCULATED,
    SYNTHESIZED,
    THIS_ONLY,
    OTHER_ONLY,
    UNSEEN,
  };

  ViewPixel(int num, Knowledge knowledge, std::optional<cv::Point2i> camera,
            const std::optional<cv::Point3d>& world)
      : num_(num), knowledge_(knowledge), camera_(camera), world_(world) {}

  int num() const { return num_; }

  Knowledge knowledge() const { return knowledge_; }
  bool visible() const {
    return knowledge_ == CALCULATED || knowledge_ == SYNTHESIZED ||
           knowledge_ == THIS_ONLY;
  }

  bool has_camera() const { return camera_.has_value(); }
  const cv::Point2i& camera() const {
    CHECK(has_camera()) << "pixel num " << num();
    return *camera_;
  }

  bool has_world() const { return world_.has_value(); }
  const cv::Point3d& world() const {
    CHECK(has_world());
    return *world_;
  }

 private:
  int num_;
  Knowledge knowledge_;

  std::optional<cv::Point2i> camera_;
  std::optional<cv::Point3d> world_;
};

#endif  // _CMD_SHOWFOUND_VIEW_PIXEL_H_
