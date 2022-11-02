#ifndef _CMD_SHOWFOUND_VIEW_PIXEL_H_
#define _CMD_SHOWFOUND_VIEW_PIXEL_H_ 1

#include <optional>

#include "opencv2/core/types.hpp"

struct ViewPixel {
  enum Knowledge {
    CALCULATED,
    SYNTHESIZED,
    THIS_ONLY,
  };

  ViewPixel(int num, cv::Point2i camera,
            const std::optional<cv::Point3d>& world, Knowledge knowledge)
      : num(num), camera(camera), world(world), knowledge(knowledge) {}

  int num;
  cv::Point2i camera;
  std::optional<cv::Point3d> world;

  Knowledge knowledge;
};

#endif  // _CMD_SHOWFOUND_VIEW_PIXEL_H_
