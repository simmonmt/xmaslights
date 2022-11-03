#include "cmd/showfound/controller.h"

#include <memory>
#include <vector>

#include "absl/log/check.h"
#include "cmd/showfound/model.h"
#include "cmd/showfound/view.h"
#include "cmd/showfound/view_pixel.h"

PixelController::PixelController(int camera_num, PixelModel& model,
                                 PixelView& view)
    : model_(model), view_(view) {
  view_.RegisterController(this);

  SetCamera(camera_num);
}

void PixelController::SetCamera(int camera_num) {
  camera_num_ = camera_num;

  auto view_pixels = std::make_unique<std::vector<ViewPixel>>();

  model_.ForEachPixel([&](const ModelPixel& pixel) {
    std::optional<cv::Point2i> camera;
    if (pixel.has_camera(camera_num)) {
      camera = pixel.camera(camera_num);
    }

    ViewPixel::Knowledge knowledge;
    if (!pixel.has_camera(camera_num)) {
      knowledge = ViewPixel::UNSEEN;
    } else if (pixel.has_world()) {
      if (pixel.synthesized()) {
        knowledge = ViewPixel::SYNTHESIZED;
      } else {
        knowledge = ViewPixel::CALCULATED;
      }
    } else {
      knowledge = ViewPixel::THIS_ONLY;
    }

    view_pixels->emplace_back(pixel.num(), knowledge, camera, pixel.world());
  });

  view_.Reset(camera_num, model_.GetRefImage(camera_num),
              std::move(view_pixels));
}
