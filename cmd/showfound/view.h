#ifndef _CMD_SHOWFOUND_VIEW_H_
#define _CMD_SHOWFOUND_VIEW_H_ 1

#include <string>
#include <unordered_map>
#include <vector>

#include "absl/types/span.h"
#include "cmd/showfound/model.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

class PixelView {
 public:
  PixelView(cv::Mat ref_image, PixelModel& model);

  ~PixelView() = default;

  void SelectNextCalculatedPixel(int dir);

  cv::Mat Render();
  int FindPixel(int x, int y);
  bool GetAndClearDirty();

  void MouseEvent(int event, cv::Point2i point);

  enum KeyboardResult {
    KEYBOARD_CONTINUE,
    KEYBOARD_QUIT,
  };
  KeyboardResult KeyboardEvent(int key);

  bool PixelIsSelected(int num);

 private:
  cv::Scalar PixelColor(const PixelModel::PixelState& pixel);
  void RenderDataBlock(cv::Mat& ui);
  void RenderOverBlock(cv::Mat& ui);
  cv::Size MaxSingleLineSize(absl::Span<const std::string> lines);
  void RenderTextBlock(cv::Mat& ui, cv::Point start, cv::Size max_line_size,
                       absl::Span<const std::string> lines);

  void SetOver(int pixel_num);
  void ClearOver();

  bool ToggleCalculatedPixel(int pixel_num);

  PixelModel& model_;
  cv::Mat ref_image_;
  cv::Mat click_map_;
  int min_pixel_num_, max_pixel_num_;
  std::vector<int> selected_;
  std::optional<int> over_;
  bool dirty_;
};

#endif  // _CMD_SHOWFOUND_VIEW_H_
