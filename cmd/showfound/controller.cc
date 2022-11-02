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
  QCHECK_EQ(1, camera_num);
  camera_num_ = camera_num;

  auto view_pixels = std::make_unique<std::vector<ViewPixel>>();

  model_.ForEachPixel(camera_num, [&](const PixelModel::PixelState& pixel) {
    ViewPixel::Knowledge knowledge;
    if (pixel.world.has_value()) {
      if (pixel.synthesized) {
        knowledge = ViewPixel::SYNTHESIZED;
      } else {
        knowledge = ViewPixel::CALCULATED;
      }
    } else {
      knowledge = ViewPixel::THIS_ONLY;
    }

    view_pixels->emplace_back(pixel.num, pixel.camera, pixel.world, knowledge);
  });

  view_.Reset(camera_num, model_.GetRefImage(camera_num),
              std::move(view_pixels));
}
