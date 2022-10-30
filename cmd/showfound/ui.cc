#include "cmd/showfound/ui.h"

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
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

PixelUI::PixelUI(cv::Mat ref_image, const std::vector<PixelState>& pixels)
    : ref_image_(ref_image), dirty_(true) {
  click_map_ = cv::Mat(ref_image.rows, ref_image.cols, CV_32S, -1);

  min_pixel_num_ = max_pixel_num_ = pixels[0].num;
  for (const auto& pixel : pixels) {
    pixels_[pixel.num] = pixel;
    min_pixel_num_ = std::min(min_pixel_num_, pixel.num);
    max_pixel_num_ = std::max(max_pixel_num_, pixel.num);

    cv::rectangle(
        click_map_,
        {std::max(pixel.coords.x - 5, 0), std::max(pixel.coords.y - 5, 0)},
        {std::min(pixel.coords.x + 5, click_map_.cols - 1),
         std::min(pixel.coords.y + 5, click_map_.rows - 1)},
        pixel.num, -1);
  }
}

void PixelUI::SelectNextCalculatedPixel(int dir) {
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

    const PixelState& state = pixels_[cur];
    if (state.selected || state.knowledge != PixelState::CALCULATED) {
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

cv::Mat PixelUI::Render() {
  cv::Mat ui = ref_image_.clone();

  for (const auto& iter : pixels_) {
    const PixelState state = iter.second;
    cv::drawMarker(ui, state.coords, PixelStateToColor(state),
                   cv::MARKER_CROSS);
  }

  RenderDataBlock(ui);
  RenderOverBlock(ui);

  return ui;
}

int PixelUI::FindPixel(int x, int y) {
  if (int pixel_num = click_map_.at<int>(y, x); pixel_num >= 0) {
    return pixel_num;
  } else {
    return -1;
  }
}

bool PixelUI::GetAndClearDirty() {
  bool dirty = dirty_;
  dirty_ = false;
  return dirty;
}

cv::Scalar PixelUI::PixelStateToColor(const PixelState& state) {
  if (state.selected) {
    return cv::viz::Color::blue();
  }

  switch (state.knowledge) {
    case PixelState::CALCULATED:
      return cv::viz::Color::green();
    case PixelState::THIS_ONLY:
      return cv::viz::Color::red();
    case PixelState::SYNTHESIZED:
      return cv::viz::Color::yellow();
  }
}

void PixelUI::RenderDataBlock(cv::Mat& ui) {
  int num_calc = 0, num_this = 0, num_syn = 0;
  std::for_each(pixels_.begin(), pixels_.end(), [&](const auto iter) {
    const PixelState& state = iter.second;
    switch (state.knowledge) {
      case PixelState::CALCULATED:
        return num_calc++;
      case PixelState::THIS_ONLY:
        return num_this++;
      case PixelState::SYNTHESIZED:
        return num_syn++;
    }
  });

  std::vector<int> ordered_selected = selected_;
  std::sort(ordered_selected.begin(), ordered_selected.end(),
            [&](int a, int b) {
              const PixelState& a_pixel = pixels_[a];
              const PixelState& b_pixel = pixels_[b];

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
    const PixelState& pixel = pixels_[num];
    lines.push_back(absl::StrFormat("%3d: %4d,%4d %6f,%6f,%6f", num,
                                    pixel.coords.x, pixel.coords.y,
                                    pixel.calc.x, pixel.calc.y, pixel.calc.z));
  }

  cv::Size max_line_size = MaxSingleLineSize(lines);
  RenderTextBlock(ui, cv::Point(0, 0), max_line_size, lines);
}

void PixelUI::RenderOverBlock(cv::Mat& ui) {
  std::string over = " ";

  if (over_.has_value()) {
    std::string type;
    const PixelState& state = pixels_[*over_];
    switch (state.knowledge) {
      case PixelState::CALCULATED:
        type = "CALC";
        break;
      case PixelState::THIS_ONLY:
        type = "THIS";
        break;
      case PixelState::SYNTHESIZED:
        type = "SYNT";
        break;
    }

    over = absl::StrFormat("%3d %s", state.num, type);
  }

  std::vector<std::string> lines = {over};
  cv::Size max_line_size = MaxSingleLineSize(lines);
  max_line_size.width = std::max(max_line_size.width, 125);

  RenderTextBlock(ui, cv::Point(ui.cols - max_line_size.width, 0),
                  max_line_size, lines);
}

void PixelUI::RenderTextBlock(cv::Mat& ui, cv::Point start,
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

cv::Size PixelUI::MaxSingleLineSize(absl::Span<const std::string> lines) {
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

void PixelUI::MouseEvent(int event, cv::Point2i point) {
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

void PixelUI::SetOver(int pixel_num) {
  if (over_.value_or(-1) != pixel_num) {
    dirty_ = true;
  }
  over_ = pixel_num;
}

void PixelUI::ClearOver() {
  if (over_.has_value()) {
    dirty_ = true;
  }
  over_.reset();
}

bool PixelUI::ToggleCalculatedPixel(int pixel_num) {
  int idx;
  for (idx = 0; idx < selected_.size(); ++idx) {
    if (selected_[idx] == pixel_num) {
      break;
    }
  }

  PixelState* state = &pixels_.find(pixel_num)->second;

  if (idx < selected_.size()) {  // already selected
    LOG(INFO) << "pixel " << state->num << " now deselected";
    selected_.erase(selected_.begin() + idx);
    state->selected = false;
    dirty_ = true;
    return true;
  }

  if (selected_.size() >= 3) {
    LOG(INFO) << "too many already selected";
    return false;
  }

  if (state->knowledge != PixelState::CALCULATED) {
    LOG(INFO) << "pixel " << state->num << " is not calculated";
    return false;
  }

  LOG(INFO) << "pixel " << state->num << " now selected";
  state->selected = true;
  selected_.push_back(pixel_num);
  dirty_ = true;
  return true;
}

PixelUI::KeyboardResult PixelUI::KeyboardEvent(int key) {
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
