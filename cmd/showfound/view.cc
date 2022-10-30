#include "cmd/showfound/view.h"

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "cmd/showfound/model.h"
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

PixelView::PixelView(cv::Mat ref_image, PixelModel& model)
    : model_(model), ref_image_(ref_image), dirty_(true) {
  click_map_ = cv::Mat(ref_image.rows, ref_image.cols, CV_32S, -1);

  min_pixel_num_ = max_pixel_num_ = -1;

  model.ForEachPixel([&](const PixelModel::PixelState& pixel) {
    if (min_pixel_num_ == -1 || pixel.num < min_pixel_num_) {
      min_pixel_num_ = pixel.num;
    }
    max_pixel_num_ = std::max(max_pixel_num_, pixel.num);

    cv::rectangle(
        click_map_,
        {std::max(pixel.coords.x - 5, 0), std::max(pixel.coords.y - 5, 0)},
        {std::min(pixel.coords.x + 5, click_map_.cols - 1),
         std::min(pixel.coords.y + 5, click_map_.rows - 1)},
        pixel.num, -1);
  });
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

    const PixelModel::PixelState& pixel = *model_.FindPixel(cur);
    if (!pixel.calc.has_value() || pixel.synthesized) {
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

  model_.ForEachPixel([&](const PixelModel::PixelState& pixel) {
    cv::drawMarker(ui, pixel.coords, PixelColor(pixel), cv::MARKER_CROSS);
    return true;
  });

  RenderDataBlock(ui);
  RenderOverBlock(ui);

  return ui;
}

int PixelView::FindPixel(int x, int y) {
  if (int pixel_num = click_map_.at<int>(y, x); pixel_num >= 0) {
    return pixel_num;
  } else {
    return -1;
  }
}

bool PixelView::GetAndClearDirty() {
  bool dirty = dirty_;
  dirty_ = false;
  return dirty;
}

cv::Scalar PixelView::PixelColor(const PixelModel::PixelState& pixel) {
  if (PixelIsSelected(pixel.num)) {
    return cv::viz::Color::blue();
  }

  if (pixel.calc.has_value()) {
    if (pixel.synthesized) {
      return cv::viz::Color::yellow();
    } else {
      return cv::viz::Color::green();
    }
  }
  return cv::viz::Color::red();
}

bool PixelView::PixelIsSelected(int pixel_num) {
  for (const int num : selected_) {
    if (num == pixel_num) {
      return true;
    }
  }
  return false;
}

void PixelView::RenderDataBlock(cv::Mat& ui) {
  int num_calc = 0, num_this = 0, num_syn = 0;
  model_.ForEachPixel([&](const PixelModel::PixelState& pixel) {
    if (pixel.calc.has_value()) {
      if (pixel.synthesized) {
        num_syn++;
      } else {
        num_calc++;
      }
      num_this++;
    }
  });

  std::vector<int> ordered_selected = selected_;
  std::sort(ordered_selected.begin(), ordered_selected.end(),
            [&](int a, int b) {
              const PixelModel::PixelState& a_pixel = *model_.FindPixel(a);
              const PixelModel::PixelState& b_pixel = *model_.FindPixel(b);

              if (int ydiff = a_pixel.coords.y - b_pixel.coords.y; ydiff < 0) {
                return true;
              } else if (ydiff > 0) {
                return false;
              }

              return a_pixel.coords.x - b_pixel.coords.y < 0;
            });

  std::vector<std::string> lines;
  lines.push_back(absl::StrFormat("%3d calc, %3d this, %3d syn", num_calc,
                                  num_this, num_syn));

  for (const int num : ordered_selected) {
    const PixelModel::PixelState& pixel = *model_.FindPixel(num);
    lines.push_back(absl::StrFormat(
        "%3d: %4d,%4d %6f,%6f,%6f", num, pixel.coords.x, pixel.coords.y,
        pixel.calc->x, pixel.calc->y, pixel.calc->z));
  }

  cv::Size max_line_size = MaxSingleLineSize(lines);
  RenderTextBlock(ui, cv::Point(0, 0), max_line_size, lines);
}

void PixelView::RenderOverBlock(cv::Mat& ui) {
  std::string over = " ";

  if (over_.has_value()) {
    std::string type;
    const PixelModel::PixelState& state = *model_.FindPixel(*over_);
    if (state.calc.has_value()) {
      if (state.synthesized) {
        type = "SYNT";
      } else {
        type = "CALC";
      }
      type = "THIS";
    }

    over = absl::StrFormat("%3d %s", state.num, type);
  }

  std::vector<std::string> lines = {over};
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
    int pixel_num = FindPixel(point.x, point.y);
    if (pixel_num >= 0) {
      ToggleCalculatedPixel(pixel_num);
    }
  } else if (event == cv::EVENT_MOUSEMOVE) {
    int pixel_num = FindPixel(point.x, point.y);
    if (pixel_num < 0) {
      ClearOver();
    } else {
      SetOver(pixel_num);
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

  const PixelModel::PixelState& pixel = *model_.FindPixel(pixel_num);

  if (idx < selected_.size()) {  // already selected
    LOG(INFO) << "pixel " << pixel.num << " now deselected";
    selected_.erase(selected_.begin() + idx);
    dirty_ = true;
    return true;
  }

  if (selected_.size() >= 3) {
    LOG(INFO) << "too many already selected";
    return false;
  }

  if (!pixel.calc.has_value() || pixel.synthesized) {
    LOG(INFO) << "pixel " << pixel.num << " is not calculated";
    return false;
  }

  LOG(INFO) << "pixel " << pixel.num << " now selected";
  selected_.push_back(pixel_num);
  dirty_ = true;
  return true;
}

PixelView::KeyboardResult PixelView::KeyboardEvent(int key) {
  switch (int key = cv::waitKey(33); key) {
    case 27:  // Escape
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
