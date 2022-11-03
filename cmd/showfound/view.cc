#include "cmd/showfound/view.h"

#include <memory>
#include <tuple>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "cmd/showfound/click_map.h"
#include "cmd/showfound/controller_view_interface.h"
#include "opencv2/opencv.hpp"
#include "opencv2/viz/types.hpp"

namespace {

constexpr int kFont = cv::FONT_HERSHEY_SIMPLEX;
constexpr double kFontScale = 0.5;
constexpr int kFontThickness = 1;
constexpr int kBorder = 10;
constexpr int kInterLineSpace = 3;
const cv::Scalar kFontColor = cv::Scalar(0, 203, 0);  // green

}  // namespace

PixelView::PixelView(int max_camera_num)
    : controller_(nullptr),
      camera_num_(0),
      max_camera_num_(max_camera_num),
      number_entry_(-1),
      min_pixel_num_(0),
      max_pixel_num_(0),
      dirty_(true) {}

void PixelView::RegisterController(ControllerViewInterface* controller) {
  QCHECK(controller_ == nullptr);
  controller_ = controller;
}

void PixelView::Reset(int camera_num, cv::Mat ref_image,
                      std::unique_ptr<std::vector<ViewPixel>> pixels) {
  all_pixels_ = std::move(pixels);

  camera_num_ = camera_num;
  ref_image_ = ref_image.clone();
  click_map_ = MakeClickMap(*all_pixels_);
  dirty_ = true;

  min_pixel_num_ = max_pixel_num_ = -1;
  pixels_.clear();
  for (int i = 0; i < all_pixels_->size(); ++i) {
    const ViewPixel* pixel = &(*all_pixels_)[i];
    if (min_pixel_num_ == -1 || pixel->num() < min_pixel_num_) {
      min_pixel_num_ = pixel->num();
    }
    max_pixel_num_ = std::max(max_pixel_num_, pixel->num());

    pixels_.emplace(pixel->num(), pixel);
  }
}

std::unique_ptr<ClickMap> PixelView::MakeClickMap(
    absl::Span<const ViewPixel> pixels) {
  std::vector<std::tuple<int, cv::Point2i>> targets;
  for (const ViewPixel& pixel : pixels) {
    if (pixel.is_seen()) {
      targets.push_back(std::make_tuple(pixel.num(), pixel.camera()));
    }
  }

  cv::Size size(ref_image_.cols, ref_image_.rows);
  return std::make_unique<ClickMap>(size, targets);
}

void PixelView::SelectNextCalculatedPixel(int dir) {
  if (selected_.empty()) {
    return;  // nothing selected
  }

  dir = dir > 0 ? 1 : -1;
  int start = *selected_.rbegin();
  int cur = start + dir;

  for (; cur != start; cur += dir) {
    if (cur > max_pixel_num_) {
      cur = min_pixel_num_;
      continue;
    }
    if (cur < min_pixel_num_) {
      cur = max_pixel_num_;
      continue;
    }

    if (PixelIsSelected(cur)) {
      continue;
    }

    const ViewPixel& pixel = *pixels_[cur];
    if (pixel.knowledge() != ViewPixel::CALCULATED) {
      continue;
    }

    break;
  }

  if (cur == start) {
    // We looped around without finding a good candidate
    return;
  }

  ToggleCalculatedPixel(start);  // turn it off
  ToggleCalculatedPixel(cur);    // turn it on
}

cv::Mat PixelView::Render() {
  cv::Mat ui = ref_image_.clone();

  for (const ViewPixel& pixel : *all_pixels_) {
    if (pixel.is_seen()) {
      cv::drawMarker(ui, pixel.camera(), PixelColor(pixel), cv::MARKER_CROSS);
    }
  }

  RenderLeftBlock(ui);
  RenderRightBlock(ui);

  return ui;
}

bool PixelView::GetAndClearDirty() {
  bool dirty = dirty_;
  dirty_ = false;
  return dirty;
}

cv::Scalar PixelView::PixelColor(const ViewPixel& pixel) {
  if (PixelIsSelected(pixel.num())) {
    return cv::viz::Color::blue();
  }

  switch (pixel.knowledge()) {
    case ViewPixel::CALCULATED:
      return cv::viz::Color::green();
    case ViewPixel::SYNTHESIZED:
      return cv::viz::Color::yellow();
    case ViewPixel::THIS_ONLY:
      return cv::viz::Color::red();
    case ViewPixel::UNSEEN:
      return cv::viz::Color::white();  // shouldn't happen
  }
}

bool PixelView::PixelIsSelected(int pixel_num) {
  for (const int num : selected_) {
    if (num == pixel_num) {
      return true;
    }
  }
  return false;
}

void PixelView::RenderLeftBlock(cv::Mat& ui) {
  int num_world = 0, num_this = 0, num_syn = 0, num_unseen = 0;
  for (const auto& [num, pixel] : pixels_) {
    switch (pixel->knowledge()) {
      case ViewPixel::CALCULATED:
        num_world++;
        break;
      case ViewPixel::SYNTHESIZED:
        num_syn++;
        break;
      case ViewPixel::THIS_ONLY:
        num_this++;
        break;
      case ViewPixel::UNSEEN:
        num_unseen++;
        break;
    }
  }

  std::vector<int> sorted_selected = selected_;
  std::sort(sorted_selected.begin(), sorted_selected.end());

  std::vector<std::string> lines;
  lines.push_back(
      absl::StrFormat("Cam %d: %3d wrld, %3d this, %3d syn %3d unsn",
                      camera_num_, num_world, num_this, num_syn, num_unseen));

  for (const int num : sorted_selected) {
    const ViewPixel& pixel = *pixels_[num];
    QCHECK_EQ(pixel.knowledge(), ViewPixel::CALCULATED);
    lines.push_back(absl::StrFormat(
        "%3d: %4d,%4d %6f,%6f,%6f", num, pixel.camera().x, pixel.camera().y,
        pixel.world().x, pixel.world().y, pixel.world().z));
  }

  cv::Size max_line_size = MaxSingleLineSize(lines);
  RenderTextBlock(ui, cv::Point(0, 0), max_line_size, lines);
}

void PixelView::RenderRightBlock(cv::Mat& ui) {
  std::string over = " ";

  if (over_.has_value()) {
    const ViewPixel& pixel = *pixels_[*over_];

    std::string type;
    switch (pixel.knowledge()) {
      case ViewPixel::CALCULATED:
        type = "CALC";
        break;
      case ViewPixel::SYNTHESIZED:
        type = "SYNT";
        break;
      case ViewPixel::THIS_ONLY:
        type = "THIS";
        break;
      case ViewPixel::UNSEEN:
        type = "UNSN";  // which would be odd
        break;
    }

    over = absl::StrFormat("%3d %s", pixel.num(), type);
  }

  std::string number = number_entry_ >= 0 ? absl::StrCat(number_entry_) : " ";

  std::vector<std::string> lines = {over, number};
  cv::Size max_line_size = MaxSingleLineSize(lines);
  max_line_size.width = std::max(max_line_size.width, 125);

  RenderTextBlock(ui, cv::Point(ui.cols - max_line_size.width, 0),
                  max_line_size, lines);
}

void PixelView::RenderTextBlock(cv::Mat& ui, cv::Point start,
                                cv::Size max_line_size,
                                absl::Span<const std::string> lines) {
  int text_start_x = start.x + kBorder;
  int text_start_y = start.y + kBorder + max_line_size.height;

  int text_total_height =
      (max_line_size.height + kInterLineSpace) * lines.size() - kInterLineSpace;

  cv::rectangle(ui, start,
                cv::Point(start.x + max_line_size.width + kBorder * 2,
                          start.y + text_total_height + kBorder * 2),
                cv::Scalar(0, 0, 0), -1);

  int text_y = text_start_y;
  for (const std::string& line : lines) {
    cv::putText(ui, line, cv::Point(text_start_x, text_y), kFont, kFontScale,
                kFontColor, kFontThickness, cv::LINE_AA);

    text_y += max_line_size.height + kInterLineSpace;
  }
}

cv::Size PixelView::MaxSingleLineSize(absl::Span<const std::string> lines) {
  int max_height = 0, max_width = 0;
  for (const std::string& line : lines) {
    int baseline;
    cv::Size size =
        cv::getTextSize(line, kFont, kFontScale, kFontThickness, &baseline);
    max_height = std::max(size.height, max_height);
    max_width = std::max(size.width, max_width);
  }

  return cv::Size(max_width, max_height);
}

void PixelView::MouseEvent(int event, cv::Point2i point) {
  if (event == cv::EVENT_LBUTTONDOWN) {
    int num = click_map_->WhichTarget(point);
    if (num < 0) {
      return;
    }

    const ViewPixel& pixel = *pixels_[num];
    if (pixel.knowledge() == ViewPixel::CALCULATED) {
      ToggleCalculatedPixel(num);
    } else if (pixel.knowledge() == ViewPixel::THIS_ONLY) {
      SynthesizePixelLocation(point);
    }

  } else if (event == cv::EVENT_MOUSEMOVE) {
    int num = click_map_->WhichTarget(point);
    if (num < 0) {
      ClearOver();
    } else {
      SetOver(num);
    }
  }
}

void PixelView::SetOver(int pixel_num) {
  if (over_.value_or(-1) != pixel_num) {
    dirty_ = true;
  }
  over_ = pixel_num;
}

void PixelView::ClearOver() {
  if (over_.has_value()) {
    dirty_ = true;
  }
  over_.reset();
}

bool PixelView::ToggleCalculatedPixel(int pixel_num) {
  int idx;
  for (idx = 0; idx < selected_.size(); ++idx) {
    if (selected_[idx] == pixel_num) {
      break;
    }
  }

  const ViewPixel& pixel = *pixels_[pixel_num];

  if (idx < selected_.size()) {  // already selected
    LOG(INFO) << "pixel " << pixel.num() << " now deselected";
    selected_.erase(selected_.begin() + idx);
    dirty_ = true;
    return true;
  }

  if (selected_.size() >= 3) {
    LOG(INFO) << "too many already selected";
    return false;
  }

  LOG(INFO) << "pixel " << pixel.num() << " now selected";
  selected_.push_back(pixel_num);
  dirty_ = true;
  return true;
}

PixelView::KeyboardResult PixelView::TrySetCamera(int camera_num) {
  if (camera_num > 0 && camera_num <= max_camera_num_) {
    controller_->SetCamera(camera_num);
  }
  return KEYBOARD_CONTINUE;
}

PixelView::KeyboardResult PixelView::KeyboardEvent(int key) {
  LOG(INFO) << "number_entry " << number_entry_ << "\n";

  if (key >= '0' && key <= '9') {
    if (number_entry_ == -1) {
      number_entry_ = 0;
    }
    number_entry_ = number_entry_ * 10 + (key - '0');
    dirty_ = true;
    return KEYBOARD_CONTINUE;
  }

  absl::Cleanup clear_number_entry = [&] {
    number_entry_ = -1;
    dirty_ = true;
  };

  switch (key) {
    case 'c':  // change camera
      return TrySetCamera(number_entry_);
    case 'q':
      return KEYBOARD_QUIT;
    case 2:  // Left arrow
      SelectNextCalculatedPixel(-1);
      break;
    case 3:  // Right arrow
      SelectNextCalculatedPixel(1);
      break;
    default:
      LOG(INFO) << "unknown key " << key;
  }
  return KEYBOARD_CONTINUE;
}

void PixelView::SynthesizePixelLocation(cv::Point2i point) {}
