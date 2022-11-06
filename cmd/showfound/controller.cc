#include "cmd/showfound/controller.h"

#include <memory>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "cmd/showfound/model.h"
#include "cmd/showfound/view.h"
#include "cmd/showfound/view_pixel.h"
#include "lib/strings/strutil.h"

PixelController::PixelController(Args args)
    : model_(args.model),
      view_(args.view),
      max_camera_num_(args.max_camera_num),
      focus_pixel_num_(-1),
      min_pixel_num_(0),
      max_pixel_num_(0),
      image_mode_(IMAGE_ALL_ON) {
  view_.RegisterController(this);

  QCHECK(model_.IsValidCameraNum(args.camera_num)) << args.camera_num;
  SetCamera(args.camera_num);
}

void PixelController::SetCamera(int camera_num) {
  if (!model_.IsValidCameraNum(camera_num)) {
    return;
  }

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
      if (pixel.has_any_camera()) {
        knowledge = ViewPixel::OTHER_ONLY;
      } else {
        knowledge = ViewPixel::UNSEEN;
      }
    } else if (pixel.has_world()) {
      knowledge = ViewPixel::CALCULATED;
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

bool PixelController::WritePixels() {
  if (absl::Status status = model_.WritePixels(); !status.ok()) {
    LOG(ERROR) << "failed to write pixels: " << status;
    return false;
  }

  return true;
}

void PixelController::PrintStatus() {
  std::cout << absl::StreamFormat("camera: %d (max %d)\n", camera_num_,
                                  max_camera_num_);
  std::cout << absl::StreamFormat(
      "focus: %s\n",
      (focus_pixel_num_ == -1 ? "none" : absl::StrCat(focus_pixel_num_)));

  std::vector<int> world, this_camera, other_camera, unknown;
  model_.ForEachPixel([&](const ModelPixel& pixel) {
    if (pixel.has_world()) {
      world.push_back(pixel.num());
    } else {
      if (pixel.has_camera(camera_num_)) {
        this_camera.push_back(pixel.num());
      } else if (pixel.has_any_camera()) {
        other_camera.push_back(pixel.num());
      } else {
        unknown.push_back(pixel.num());
      }
    }
  });

  std::cout << "pixels:\n";
  std::cout << absl::StreamFormat("  world: %s\n", IndexesToRanges(world));
  std::cout << absl::StreamFormat("  this : %s\n",
                                  IndexesToRanges(this_camera));
  std::cout << absl::StreamFormat("  other: %s\n",
                                  IndexesToRanges(other_camera));
  std::cout << absl::StreamFormat("  unk  : %s\n", IndexesToRanges(unknown));
}
