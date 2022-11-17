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
#include "cmd/showfound/view_command.h"
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

PixelView::PixelView()
    : controller_(nullptr),
      camera_num_(0),
      image_mode_(IMAGE_ALL_ON),
      skip_mode_(EVERY_PIXEL),
      keymap_(MakeKeymap()),
      show_crosshairs_(false),
      dirty_(true) {}

void PixelView::RegisterController(ControllerViewInterface* controller) {
  QCHECK(controller_ == nullptr);
  controller_ = controller;
}

void PixelView::Reset(int camera_num, cv::Mat background_image,
                      const std::vector<const ViewPixel*>& pixels) {
  camera_num_ = camera_num;
  SetBackgroundImage(background_image);

  all_pixels_.clear();
  for (const ViewPixel* pixel : pixels) {
    all_pixels_[pixel->num()] = pixel;
  }
  ShowAllPixels();
  dirty_ = true;
}

void PixelView::ShowAllPixels() {
  focused_pixel_.reset();
  UpdateClickMap();
  dirty_ = true;
}

void PixelView::FocusOnPixel(int pixel_num) {
  focused_pixel_ = pixel_num;
  UpdateClickMap();
  dirty_ = true;
}

void PixelView::UpdatePixel(const ViewPixel& pixel) {
  all_pixels_[pixel.num()] = &pixel;
  dirty_ = true;
}

void PixelView::SetImageMode(ImageMode image_mode) {
  image_mode_ = image_mode;
  dirty_ = true;
}

void PixelView::SetSkipMode(SkipMode skip_mode) {
  skip_mode_ = skip_mode;
  dirty_ = true;
}

void PixelView::SetBackgroundImage(cv::Mat background_image) {
  background_image_ = background_image;
}

std::pair<std::map<int, const ViewPixel*>::const_iterator,
          std::map<int, const ViewPixel*>::const_iterator>
PixelView::VisiblePixels() {
  if (focused_pixel_.has_value()) {
    std::map<int, const ViewPixel*>::const_iterator iter =
        all_pixels_.find(*focused_pixel_);
    auto next = iter;
    return std::make_tuple(iter, ++next);
  }

  return std::make_tuple(all_pixels_.cbegin(), all_pixels_.cend());
}

void PixelView::UpdateClickMap() {
  std::vector<std::tuple<int, cv::Point2i>> targets;
  for (auto [cur, end] = VisiblePixels(); cur != end; ++cur) {
    const auto& [num, pixel] = *cur;
    if (pixel->visible()) {
      targets.push_back(std::make_tuple(num, pixel->camera()));
    }
  }

  cv::Size size(background_image_.cols, background_image_.rows);
  click_map_ = std::make_unique<ClickMap>(size, targets);
}

cv::Mat PixelView::Render() {
  cv::Mat ui = background_image_.clone();

  for (const auto& [num, pixel] : all_pixels_) {
    if (!pixel->has_camera()) {
      continue;
    }

    if (selected_pixels_.find(num) != selected_pixels_.end()) {
      cv::drawMarker(ui, pixel->camera(), cv::viz::Color::blue(),
                     cv::MARKER_TILTED_CROSS);
    } else if (!focused_pixel_.has_value() || *focused_pixel_ == num) {
      cv::drawMarker(ui, pixel->camera(), PixelColor(*pixel),
                     cv::MARKER_TILTED_CROSS);
    }
  }

  RenderLeftBlock(ui);
  RenderRightBlock(ui);

  if (show_crosshairs_) {
    cv::drawMarker(ui, mouse_pos_, cv::viz::Color::red(), cv::MARKER_CROSS, 20,
                   2);
  }

  return ui;
}

bool PixelView::GetAndClearDirty() {
  bool dirty = dirty_;
  dirty_ = false;
  return dirty;
}

cv::Scalar PixelView::PixelColor(const ViewPixel& pixel) {
  switch (pixel.knowledge()) {
    case ViewPixel::CALCULATED:
      return cv::viz::Color::green();
    case ViewPixel::SYNTHESIZED:
      return cv::viz::Color::yellow();
    case ViewPixel::THIS_ONLY:
      return cv::viz::Color::red();
    case ViewPixel::OTHER_ONLY:
    case ViewPixel::UNSEEN:
      return cv::viz::Color::white();  // shouldn't happen
  }
}

namespace {

std::string ShortKnowledge(ViewPixel::Knowledge knowledge) {
  switch (knowledge) {
    case ViewPixel::CALCULATED:
      return "CALC";
    case ViewPixel::SYNTHESIZED:
      return "SYNT";
    case ViewPixel::THIS_ONLY:
      return "THIS";
    case ViewPixel::OTHER_ONLY:
      return "OTHR";
    case ViewPixel::UNSEEN:
      return "UNSN";
  }
}

std::string PixelInfo(const ViewPixel& pixel) {
  return absl::StrFormat("%3d %s", pixel.num(),
                         ShortKnowledge(pixel.knowledge()));
}

std::string ShortImageMode(ImageMode mode) {
  switch (mode) {
    case IMAGE_ALL_ON:
      return "ON";
    case IMAGE_ALL_OFF:
      return "OFF";
    case IMAGE_FOCUS_ON:
      return "FOCUS";
    case IMAGE_LAST:
      break;
  }
  return "???";
}

std::string ShortSkipMode(SkipMode mode) {
  switch (mode) {
    case EVERY_PIXEL:
      return "EVERY";
    case ONLY_OTHER:
      return "OTHER";
    case ONLY_UNKNOWN:
      return "UNK";
    case SKIP_LAST:
      break;
  }
  return "???";
}

}  // namespace

void PixelView::RenderLeftBlock(cv::Mat& ui) {
  int num_world = 0, num_syn = 0, num_this = 0, num_other = 0, num_unseen = 0;
  for (const auto& [num, pixel] : all_pixels_) {
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
      case ViewPixel::OTHER_ONLY:
        num_other++;
        break;
      case ViewPixel::UNSEEN:
        num_unseen++;
        break;
    }
  }

  std::vector<std::string> lines;
  lines.push_back(absl::StrFormat(
      "Cam %d: %3d wrld %3d syn %3d this %3d othr %3d unsn", camera_num_,
      num_world, num_syn, num_this, num_other, num_unseen));

  lines.push_back(absl::StrFormat("Img: %s, Skip: %s",
                                  ShortImageMode(image_mode_),
                                  ShortSkipMode(skip_mode_)));

  cv::Size max_line_size = MaxSingleLineSize(lines);
  RenderTextBlock(ui, cv::Point(0, 0), max_line_size, lines);
}

void PixelView::RenderRightBlock(cv::Mat& ui) {
  std::optional<int> to_describe;
  bool focus = false;
  if (focused_pixel_.has_value()) {
    to_describe = *focused_pixel_;
    focus = true;
  } else if (over_.has_value()) {
    to_describe = *over_;
  }

  std::string info = " ";
  if (to_describe) {
    bool selected =
        selected_pixels_.find(*to_describe) != selected_pixels_.end();

    info = absl::StrCat((focus ? "F: " : ""),
                        PixelInfo(*all_pixels_[*to_describe]),
                        (selected ? " SEL" : ""));
  }

  std::string command = command_buffer_.ToString();
  if (command.empty()) {
    command = " ";
  }

  std::vector<std::string> lines = {info, command};
  cv::Size max_line_size = MaxSingleLineSize(lines);
  max_line_size.width = std::max(max_line_size.width, 200);

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
    mouse_pos_ = point;
    command_buffer_.AddClick();
    TryExecuteCommand();

  } else if (event == cv::EVENT_MOUSEMOVE) {
    int num = click_map_->WhichTarget(point);
    if (num < 0) {
      ClearOver();
    } else {
      SetOver(num);
    }

    if (show_crosshairs_) {
      dirty_ = true;
    }
    mouse_pos_ = point;
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

void PixelView::SetSelectedPixels(const std::set<int>& selected_pixels) {
  selected_pixels_ = selected_pixels;
  dirty_ = true;
}

PixelView::KeyboardResult PixelView::KeyboardEvent(int key) {
  if (key == 'q') {
    return KEYBOARD_QUIT;
  }

  dirty_ = true;
  show_crosshairs_ = false;

  if (key == kEscapeKey) {
    command_buffer_.Reset();
    return KEYBOARD_CONTINUE;
  }

  command_buffer_.AddKey(key);
  TryExecuteCommand();
  return KEYBOARD_CONTINUE;
}

void PixelView::TryExecuteCommand() {
  std::variant<Keymap::LookupResult, const Command*> var =
      keymap_->Lookup(command_buffer_);
  if (auto* result = std::get_if<Keymap::LookupResult>(&var);
      result != nullptr) {
    switch (*result) {
      case Keymap::UNKNOWN:
        command_buffer_.SetError(CommandBuffer::UNKNOWN);
        show_crosshairs_ = false;
        return;
      case Keymap::CONTINUE:
        return;  // more keys to gather
    }
    LOG(FATAL) << "unreachable";
  }

  const Command& command = *std::get<const Command*>(var);
  switch (command.Evaluate(command_buffer_)) {
    case Command::NEED_CLICK:
      show_crosshairs_ = true;
      return;
    case Command::EVAL_OK:
      break;
  }

  Command::Args args = {
      .prefix = command_buffer_.prefix(),
      .focus = focused_pixel_,
      .over = over_,
      .mouse_coords = mouse_pos_,
  };

  show_crosshairs_ = false;
  command_buffer_.Reset();
  dirty_ = true;

  switch (command.Execute(args)) {
    case Command::USAGE:
      command_buffer_.SetError(CommandBuffer::USAGE);
      std::cerr << "Incorrect command; proper trigger: "
                << command.DescribeTrigger() << "\n";
      return;
    case Command::ERROR:
      std::cerr << "Command failed: " << command.help()
                << "\n";  // TODO: better than this
      return;
    case Command::EXEC_OK:
      break;
  }
}

namespace {

std::function<Command::ExecuteResult()> NoFail(std::function<void()> func) {
  return [func] {
    func();
    return Command::EXEC_OK;
  };
}

Command::ExecuteResult OkOrError(bool rc) {
  return rc ? Command::EXEC_OK : Command::ERROR;
}

}  // namespace

std::unique_ptr<const Keymap> PixelView::MakeKeymap() {
  auto keymap = std::make_unique<Keymap>();

  // builtins:
  //    q: quit
  //  ESC: reset command buffer
  //    ?: help

  keymap->Add(std::make_unique<BareCommand>(
      'a', "view all pixels", NoFail([&] { controller_->Unfocus(); })));
  keymap->Add(std::make_unique<ArgCommand>(
      'f', "focus on one pixel", ArgCommand::PREFIX | ArgCommand::OVER,
      ArgCommand::PREFER, [&](int pixel_num) {
        controller_->Focus(pixel_num);
        return Command::EXEC_OK;
      }));

  keymap->Add(std::make_unique<ArgCommand>(
      'c', "select camera", ArgCommand::PREFIX, ArgCommand::PREFER,  //
      [&](int camera_num) {
        controller_->SetCamera(camera_num);
        return Command::EXEC_OK;
      }));
  keymap->Add(std::make_unique<BareCommand>(
      'i', "next image mode", NoFail([&] { controller_->NextImageMode(); })));
  keymap->Add(std::make_unique<BareCommand>(
      'k', "change sKip mode", NoFail([&] { controller_->NextSkipMode(); })));

  keymap->Add(std::make_unique<ClickCommand>(
      'p', "set (place) pixel location", ArgCommand::PREFIX | ArgCommand::FOCUS,
      ArgCommand::EXCLUSIVE, [&](cv::Point2i location, int num) {
        return OkOrError(controller_->SetPixelLocation(num, location));
      }));

  keymap->Add(std::make_unique<BareCommand>(
      's', "status", NoFail([&] { controller_->PrintStatus(); })));
  keymap->Add(std::make_unique<BareCommand>('w', "write pixels", [&] {
    return OkOrError(controller_->WritePixels());
  }));

  keymap->Add(std::make_unique<BareCommand>(
      kLeftArrowKey, "previous pixel",
      NoFail([&] { controller_->NextPixel(false); })));
  keymap->Add(std::make_unique<BareCommand>(
      kRightArrowKey, "next pixel",
      NoFail([&] { controller_->NextPixel(true); })));

  keymap->Add(std::make_unique<ClickCommand>(
      kLeftMouseButton, "select a pixel", ArgCommand::OVER,
      ArgCommand::EXCLUSIVE, [&](cv::Point2i location, int num) {
        return OkOrError(controller_->SelectPixel(num));
      }));
  keymap->Add(std::make_unique<ArgCommand>(
      ' ', "select a pixel",
      ArgCommand::PREFIX | ArgCommand::FOCUS | ArgCommand::OVER,
      ArgCommand::PREFER, [&](int num) {
        controller_->SelectPixel(num);
        return Command::EXEC_OK;
      }));
  keymap->Add(std::make_unique<BareCommand>(
      'C', "clear pixel selections",
      NoFail([&] { controller_->ClearSelectedPixels(); })));

  return keymap;
}
