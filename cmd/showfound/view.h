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
#include "cmd/showfound/view_pixel.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

class PixelView {
 public:
  PixelView(int max_camera_num);
  ~PixelView() = default;

  void RegisterController(ControllerViewInterface* controller);

  void Reset(int camera_num, cv::Mat ref_image,
             std::unique_ptr<std::vector<ViewPixel>> pixels);

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

 private:
  std::unique_ptr<ClickMap> MakeClickMap(absl::Span<const ViewPixel> pixels);

  cv::Scalar PixelColor(const ViewPixel& pixel);
  void RenderLeftBlock(cv::Mat& ui);
  void RenderRightBlock(cv::Mat& ui);
  cv::Size MaxSingleLineSize(absl::Span<const std::string> lines);
  void RenderTextBlock(cv::Mat& ui, cv::Point start, cv::Size max_line_size,
                       absl::Span<const std::string> lines);

  void SetOver(int pixel_num);
  void ClearOver();

  bool ToggleCalculatedPixel(int pixel_num);
  void SynthesizePixelLocation(cv::Point2i point);

  KeyboardResult TrySetCamera(int camera_num);

  ControllerViewInterface* controller_;  // not owned
  int camera_num_;
  const int max_camera_num_;
  int number_entry_;
  cv::Mat ref_image_;
  std::unique_ptr<ClickMap> click_map_;
  std::unique_ptr<std::vector<ViewPixel>> all_pixels_;
  std::unordered_map<int, const ViewPixel*> pixels_;

  int min_pixel_num_, max_pixel_num_;
  std::vector<int> selected_;
  std::optional<int> over_;
  bool dirty_;
};

#endif  // _CMD_SHOWFOUND_VIEW_H_
