#ifndef _CMD_SHOWFOUND_UI_H_
#define _CMD_SHOWFOUND_UI_H_ 1

#include <string>
#include <unordered_map>
#include <vector>

#include "absl/types/span.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

struct PixelState {
  int num;
  cv::Point2i coords;
  cv::Point3d calc;

  enum Knowledge {
    CALCULATED = 1,
    THIS_ONLY = 2,
    SYNTHESIZED = 3,
  };
  Knowledge knowledge;

  bool selected;
};

class PixelUI {
 public:
  PixelUI(cv::Mat ref_image, const std::vector<PixelState>& pixels);

  ~PixelUI() = default;

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

 private:
  cv::Scalar PixelStateToColor(const PixelState& state);
  void RenderDataBlock(cv::Mat& ui);
  void RenderOverBlock(cv::Mat& ui);
  cv::Size MaxSingleLineSize(absl::Span<const std::string> lines);
  void RenderTextBlock(cv::Mat& ui, cv::Point start, cv::Size max_line_size,
                       absl::Span<const std::string> lines);

  void SetOver(int pixel_num);
  void ClearOver();

  bool ToggleCalculatedPixel(int pixel_num);

  cv::Mat ref_image_;
  cv::Mat click_map_;
  int min_pixel_num_, max_pixel_num_;
  std::unordered_map<int, PixelState> pixels_;
  std::vector<int> selected_;
  std::optional<int> over_;
  bool dirty_;
};

#endif  // _CMD_SHOWFOUND_UI_H_
