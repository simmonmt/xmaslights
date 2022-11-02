#ifndef _CMD_SHOWFOUND_VIEW_H_
#define _CMD_SHOWFOUND_VIEW_H_ 1

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "absl/types/span.h"
#include "cmd/showfound/click_map.h"
#include "cmd/showfound/model.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

struct ViewPixel {
  ViewPixel(int num, cv::Point2i camera,
            const std::optional<cv::Point3d>& world)
      : num(num), camera(camera), world(world) {}

  int num;
  cv::Point2i camera;
  std::optional<cv::Point3d> world;

  enum Knowledge {
    CALCULATED,
    SYNTHESIZED,
    THIS_ONLY,
  };
  Knowledge knowledge;
};

class PixelView {
 public:
  PixelView();
  ~PixelView() = default;

  void Init(cv::Mat ref_image, absl::Span<const ViewPixel> pixels);

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
  void RenderDataBlock(cv::Mat& ui);
  void RenderOverBlock(cv::Mat& ui);
  cv::Size MaxSingleLineSize(absl::Span<const std::string> lines);
  void RenderTextBlock(cv::Mat& ui, cv::Point start, cv::Size max_line_size,
                       absl::Span<const std::string> lines);

  void SetOver(int pixel_num);
  void ClearOver();

  bool ToggleCalculatedPixel(int pixel_num);
  void SynthesizePixelLocation(cv::Point2i point);

  cv::Mat ref_image_;
  std::unique_ptr<ClickMap> click_map_;
  std::unordered_map<int, const ViewPixel*> pixels_;

  int min_pixel_num_, max_pixel_num_;
  std::vector<int> selected_;
  std::optional<int> over_;
  bool dirty_;
};

#endif  // _CMD_SHOWFOUND_VIEW_H_
