#ifndef _CMD_SHOWFOUND_VIEW_H_
#define _CMD_SHOWFOUND_VIEW_H_ 1

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "absl/types/span.h"
#include "cmd/showfound/click_map.h"
#include "cmd/showfound/controller_view_interface.h"
#include "cmd/showfound/view_command.h"
#include "cmd/showfound/view_pixel.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

class PixelView {
 public:
  PixelView();
  ~PixelView() = default;

  void RegisterController(ControllerViewInterface* controller);

  void Reset(int camera_num, cv::Mat background_image,
             absl::Span<const ViewPixel> pixels);

  void SetBackgroundImage(cv::Mat background_image);
  void ShowPixel(int pixel_num);
  void ShowAllPixels();

  void SelectNextCalculatedPixel(int dir);

  cv::Mat Render();
  bool GetAndClearDirty();

  void MouseEvent(int event, cv::Point2i point);

  enum KeyboardResult {
    KEYBOARD_CONTINUE,
    KEYBOARD_QUIT,
  };
  KeyboardResult KeyboardEvent(int key);

  bool PixelIsSelected(int num);

  void PrintHelp();

 private:
  std::unique_ptr<const Keymap> MakeKeymap();
  void TryExecuteCommand();

  void SetVisiblePixels(absl::Span<const ViewPixel> pixels);
  std::unique_ptr<ClickMap> MakeClickMap(absl::Span<const ViewPixel> pixels);
  std::optional<int> FocusedPixel();

  cv::Scalar PixelColor(const ViewPixel& pixel);
  void RenderLeftBlock(cv::Mat& ui);
  void RenderRightBlock(cv::Mat& ui);
  cv::Size MaxSingleLineSize(absl::Span<const std::string> lines);
  void RenderTextBlock(cv::Mat& ui, cv::Point start, cv::Size max_line_size,
                       absl::Span<const std::string> lines);

  void SetOver(int pixel_num);
  void ClearOver();

  bool ToggleCalculatedPixel(int pixel_num);

  bool NewPixel(int pixel_num, cv::Point2i location);
  bool MovePixel(int pixel_num, cv::Point2i location);

  ControllerViewInterface* controller_;  // not owned
  int camera_num_;
  cv::Mat background_image_;
  std::unique_ptr<ClickMap> click_map_;
  absl::Span<const ViewPixel> all_pixels_;  // all pixels for camera
  absl::Span<const ViewPixel> pixels_;      // visible pixels
  std::unordered_map<int, const ViewPixel*> pixels_by_num_;  // visible by num

  int min_pixel_num_, max_pixel_num_;
  std::vector<int> selected_;

  std::optional<int> over_;
  cv::Point2i mouse_pos_;

  std::unique_ptr<const Keymap> keymap_;
  CommandBuffer command_buffer_;
  bool show_crosshairs_;

  bool dirty_;
};

#endif  // _CMD_SHOWFOUND_VIEW_H_
