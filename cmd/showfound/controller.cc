#include "cmd/showfound/controller.h"

#include <memory>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "cmd/showfound/model.h"
#include "cmd/showfound/view.h"
#include "cmd/showfound/view_pixel.h"

PixelController::PixelController(int camera_num, PixelModel& model,
                                 PixelView& view)
    : model_(model),
      view_(view),
      focus_pixel_num_(-1),
      min_pixel_num_(0),
      max_pixel_num_(0),
      image_mode_(IMAGE_ALL_ON) {
  view_.RegisterController(this);

  SetCamera(camera_num);
}

void PixelController::SetCamera(int camera_num) {
  camera_num_ = camera_num;
  image_mode_ = IMAGE_ALL_ON;

  auto camera_pixels = std::make_unique<std::vector<ViewPixel>>();

  min_pixel_num_ = max_pixel_num_ = -1;
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

    if (min_pixel_num_ == -1 || pixel.num() < min_pixel_num_) {
      min_pixel_num_ = pixel.num();
    }
    max_pixel_num_ = std::max(max_pixel_num_, pixel.num());

    camera_pixels->emplace_back(pixel.num(), knowledge, camera, pixel.world());
  });

  view_.Reset(camera_num, ViewBackgroundImage(), *camera_pixels);
  camera_pixels_ = std::move(camera_pixels);
}

void PixelController::NextImageMode() {
  ImageMode next = static_cast<ImageMode>(static_cast<int>(image_mode_) + 1);
  if (next == IMAGE_LAST) {
    next = IMAGE_ALL_ON;
  }
  SetImageMode(next);
}

void PixelController::SetImageMode(ImageMode mode) {
  image_mode_ = mode;
  view_.SetBackgroundImage(ViewBackgroundImage());
}

cv::Mat PixelController::ViewBackgroundImage() {
  switch (image_mode_) {
    case IMAGE_ALL_ON:
      return model_.GetAllOnImage(camera_num_);
    case IMAGE_ALL_OFF:
      return model_.GetAllOffImage(camera_num_);
    case IMAGE_FOCUS_ON: {
      if (focus_pixel_num_ == -1) {
        return model_.GetAllOffImage(camera_num_);
      }

      absl::StatusOr<cv::Mat> image =
          model_.GetPixelOnImage(camera_num_, focus_pixel_num_);
      if (!image.ok()) {
        LOG(ERROR) << "failed to load cam " << camera_num_ << " pixel "
                   << focus_pixel_num_ << ": " << image.status();
        return model_.GetAllOffImage(camera_num_);
      }

      return *image;
    } break;
    case IMAGE_LAST:
      QCHECK(false) << "shouldn't happen";
  }
}

void PixelController::Unfocus() {
  focus_pixel_num_ = -1;
  SetImageMode(IMAGE_ALL_ON);
  view_.ShowAllPixels();
}

void PixelController::Focus(int pixel_num) {
  focus_pixel_num_ = pixel_num;
  view_.ShowPixel(pixel_num);
}

void PixelController::NextPixel(bool forward) {
  int pixel_num = focus_pixel_num_ + (forward ? 1 : -1);
  if (pixel_num == min_pixel_num_) {
    pixel_num = max_pixel_num_;
  } else if (pixel_num == max_pixel_num_) {
    pixel_num = min_pixel_num_;
  }

  Focus(pixel_num);
}
