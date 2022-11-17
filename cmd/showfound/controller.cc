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
      solver_(args.solver),
      max_camera_num_(args.max_camera_num),
      min_pixel_num_(0),
      max_pixel_num_(0),
      image_mode_(IMAGE_ALL_ON),
      skip_mode_(EVERY_PIXEL) {
  view_.RegisterController(this);

  QCHECK(model_.IsValidCameraNum(args.camera_num)) << args.camera_num;
  SetCamera(args.camera_num);
}

std::unique_ptr<ViewPixel> PixelController::ModelToViewPixel(
    const ModelPixel& model_pixel, int camera_num) {
  std::optional<cv::Point2i> camera;
  if (model_pixel.has_camera(camera_num)) {
    camera = model_pixel.camera(camera_num);
  }

  ViewPixel::Knowledge knowledge;
  if (!model_pixel.has_camera(camera_num)) {
    if (model_pixel.has_any_camera()) {
      knowledge = ViewPixel::OTHER_ONLY;
    } else {
      knowledge = ViewPixel::UNSEEN;
    }
  } else if (model_pixel.has_world()) {
    knowledge = ViewPixel::CALCULATED;
  } else {
    knowledge = ViewPixel::THIS_ONLY;
  }

  return std::make_unique<ViewPixel>(model_pixel.num(), knowledge, camera,
                                     model_pixel.world());
}

void PixelController::SetCamera(int camera_num) {
  if (!model_.IsValidCameraNum(camera_num)) {
    return;
  }

  camera_num_ = camera_num;
  image_mode_ = IMAGE_ALL_ON;

  auto camera_pixels =
      std::make_unique<std::vector<std::unique_ptr<ViewPixel>>>();
  std::vector<const ViewPixel*> pixels_for_view;

  min_pixel_num_ = max_pixel_num_ = -1;
  model_.ForEachPixel([&](const ModelPixel& model_pixel) {
    if (min_pixel_num_ == -1 || model_pixel.num() < min_pixel_num_) {
      min_pixel_num_ = model_pixel.num();
    }
    max_pixel_num_ = std::max(max_pixel_num_, model_pixel.num());

    std::unique_ptr<ViewPixel> view_pixel =
        ModelToViewPixel(model_pixel, camera_num);
    pixels_for_view.push_back(view_pixel.get());
    camera_pixels->push_back(std::move(view_pixel));
  });

  view_.Reset(camera_num, ViewBackgroundImage(), pixels_for_view);
  camera_pixels_ = std::move(camera_pixels);
}

void PixelController::NextImageMode() {
  ImageMode next = static_cast<ImageMode>(static_cast<int>(image_mode_) + 1);
  if (!focus_pixel_num_.has_value() && next == IMAGE_FOCUS_ON) {
    next = static_cast<ImageMode>(static_cast<int>(next) + 1);
  }
  if (next == IMAGE_LAST) {
    next = IMAGE_ALL_ON;
  }
  SetImageMode(next);
}

void PixelController::SetImageMode(ImageMode mode) {
  image_mode_ = mode;
  view_.SetImageMode(image_mode_);
  view_.SetBackgroundImage(ViewBackgroundImage());
}

void PixelController::NextSkipMode() {
  SkipMode next = static_cast<SkipMode>(static_cast<int>(skip_mode_) + 1);
  if (next == SKIP_LAST) {
    next = EVERY_PIXEL;
  }
  SetSkipMode(next);
}

void PixelController::SetSkipMode(SkipMode skip_mode) {
  skip_mode_ = skip_mode;
  view_.SetSkipMode(skip_mode_);
}

cv::Mat PixelController::ViewBackgroundImage() {
  switch (image_mode_) {
    case IMAGE_ALL_ON:
      return model_.GetAllOnImage(camera_num_);
    case IMAGE_ALL_OFF:
      return model_.GetAllOffImage(camera_num_);
    case IMAGE_FOCUS_ON: {
      if (!focus_pixel_num_.has_value()) {
        return model_.GetAllOffImage(camera_num_);
      }

      absl::StatusOr<cv::Mat> image =
          model_.GetPixelOnImage(camera_num_, *focus_pixel_num_);
      if (!image.ok()) {
        LOG(ERROR) << "failed to load cam " << camera_num_ << " pixel "
                   << *focus_pixel_num_ << ": " << image.status();
        return model_.GetAllOffImage(camera_num_);
      }

      return *image;
    } break;
    case IMAGE_LAST:
      QCHECK(false) << "shouldn't happen";
  }
}

void PixelController::Unfocus() {
  focus_pixel_num_.reset();
  SetImageMode(IMAGE_ALL_ON);
  view_.ShowAllPixels();
}

void PixelController::Focus(int pixel_num) {
  focus_pixel_num_ = pixel_num;
  view_.SetBackgroundImage(ViewBackgroundImage());
  view_.FocusOnPixel(pixel_num);
}

void PixelController::NextPixel(bool forward) {
  if (!focus_pixel_num_.has_value()) {
    return;
  }

  int pixel_num = *focus_pixel_num_;
  for (;;) {
    pixel_num += (forward ? 1 : -1);
    if (pixel_num <= min_pixel_num_) {
      pixel_num = max_pixel_num_;
    } else if (pixel_num >= max_pixel_num_) {
      pixel_num = min_pixel_num_;
    }

    if (skip_mode_ == EVERY_PIXEL || pixel_num == *focus_pixel_num_) {
      break;
    }

    bool has_this_camera = false;
    bool has_other_cameras = false;
    const ModelPixel& pixel = *model_.FindPixel(pixel_num);
    for (const int& camera_num : pixel.cameras()) {
      if (camera_num == camera_num_) {
        has_this_camera = true;
      } else {
        has_other_cameras = true;
      }
    }

    if (skip_mode_ == ONLY_OTHER && !has_this_camera && has_other_cameras) {
      break;
    }
    if (skip_mode_ == ONLY_UNKNOWN && !has_this_camera && !has_other_cameras) {
      break;
    }
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
      (focus_pixel_num_.has_value() ? absl::StrCat(*focus_pixel_num_)
                                    : "none"));

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
  std::cout << absl::StreamFormat("  world %3d: %s\n", world.size(),
                                  IndexesToRanges(world));
  std::cout << absl::StreamFormat("  this  %3d: %s\n", this_camera.size(),
                                  IndexesToRanges(this_camera));
  std::cout << absl::StreamFormat("  other %3d: %s\n", other_camera.size(),
                                  IndexesToRanges(other_camera));
  std::cout << absl::StreamFormat("  unk   %3d: %s\n", unknown.size(),
                                  IndexesToRanges(unknown));
}

namespace {

// TODO: figure out why the stream converters aren't working
std::string PointToString(const cv::Point3d& point) {
  return absl::StrFormat("%f,%f,%f", point.x, point.y, point.z);
}

}  // namespace

bool PixelController::SetPixelLocation(int pixel_num, cv::Point2i location) {
  if (!selected_pixels_.empty()) {
    LOG(ERROR) << "Can't set pixel location with selected pixels";
    return false;
  }

  LOG(INFO) << "set pixel " << pixel_num << " location to " << location.x << ","
            << location.y;

  const ModelPixel& existing_pixel = *model_.FindPixel(pixel_num);
  const bool needs_recalc = existing_pixel.has_other_camera(camera_num_);

  ModelPixelBuilder pixel_builder =
      ModelPixelBuilder(existing_pixel)
          .SetCameraLocation(camera_num_, location, true);

  ModelPixel new_pixel = pixel_builder.Build();
  if (needs_recalc) {
    LOG(INFO) << "pixel " << pixel_num << " has other cameras; recalculating";

    cv::Point3d new_world = solver_.CalculateWorldLocation(new_pixel);
    pixel_builder.SetWorldLocation(new_world);
    new_pixel = pixel_builder.Build();

    if (existing_pixel.has_world()) {
      LOG(INFO) << "old world: " << PointToString(existing_pixel.world())
                << " now " << PointToString(new_world);
    } else {
      LOG(INFO) << "world: " << PointToString(new_world);
    }

  } else {
    LOG(INFO) << "pixel " << pixel_num << " doesn't need recalculation";
  }

  if (!model_.UpdatePixel(pixel_num, new_pixel)) {
    LOG(ERROR) << "failed to update model";
    return false;
  }

  UpdatePixel(pixel_num);

  return true;
}

void PixelController::UpdatePixel(int pixel_num) {
  const ModelPixel& model_pixel = *model_.FindPixel(pixel_num);
  std::unique_ptr<ViewPixel> view_pixel =
      ModelToViewPixel(model_pixel, camera_num_);

  view_.UpdatePixel(*view_pixel);

  for (int i = 0; i < camera_pixels_->size(); ++i) {
    if ((*camera_pixels_)[i]->num() == pixel_num) {
      (*camera_pixels_)[i] = std::move(view_pixel);
    }
  }
}

bool PixelController::SelectPixel(int pixel_num) {
  if (auto iter = selected_pixels_.find(pixel_num);
      iter != selected_pixels_.end()) {
    selected_pixels_.erase(iter);
    view_.SetSelectedPixels(selected_pixels_);
    return true;
  }

  if (selected_pixels_.size() == 3) {
    LOG(ERROR) << "At most three pixels may be selected";
    return false;
  }

  const ModelPixel& model_pixel = *model_.FindPixel(pixel_num);
  if (!model_pixel.has_world()) {
    LOG(ERROR) << "Only pixels with world coordinates may be selected";
    return false;
  }

  selected_pixels_.insert(pixel_num);
  view_.SetSelectedPixels(selected_pixels_);
  return true;
}

void PixelController::ClearSelectedPixels() {
  selected_pixels_.clear();
  view_.SetSelectedPixels(selected_pixels_);
}
