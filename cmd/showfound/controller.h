#ifndef _CMD_SHOWFOUND_CONTROLLER_H_
#define _CMD_SHOWFOUND_CONTROLLER_H_ 1

#include <memory>
#include <optional>
#include <set>

#include "cmd/showfound/common.h"
#include "cmd/showfound/controller_view_interface.h"
#include "cmd/showfound/model.h"
#include "cmd/showfound/solver.h"
#include "cmd/showfound/view.h"
#include "cmd/showfound/view_pixel.h"
#include "opencv2/core/types.hpp"

class PixelController : public ControllerViewInterface {
 public:
  struct Args {
    int camera_num;
    int max_camera_num;
    PixelModel& model;
    PixelView& view;
    PixelSolver& solver;
  };

  PixelController(Args args);

  void SetCamera(int camera_num) override;
  void NextImageMode() override;
  void NextSkipMode() override;
  void Unfocus() override;
  bool Focus(int pixel_num) override;
  void NextPixel(bool forward) override;
  void PrintStatus() override;
  bool WritePixels() override;
  bool SetPixelLocation(int pixel_num, cv::Point2i location) override;
  bool RemovePixelLocation(int pixel_num) override;
  bool SynthesizeWorldLocation(int pixel_num) override;
  bool SelectPixel(int pixel_num) override;
  void ClearSelectedPixels() override;

 private:
  std::unique_ptr<ViewPixel> ModelToViewPixel(const ModelPixel& model_pixel,
                                              int camera_num);
  void SetImageMode(ImageMode mode);
  void SetSkipMode(SkipMode skip_mode);
  cv::Mat ViewBackgroundImage();

  bool IsValidCameraNum(int camera_num);

  void UpdatePixel(int pixel_num);

  PixelModel& model_;
  PixelView& view_;
  PixelSolver& solver_;

  int camera_num_;
  int max_camera_num_;
  std::optional<int> focus_pixel_num_;
  int min_pixel_num_, max_pixel_num_;
  ImageMode image_mode_;
  SkipMode skip_mode_;
  std::set<int> selected_pixels_;

  std::unique_ptr<std::vector<std::unique_ptr<ViewPixel>>> camera_pixels_;
};

#endif  // _CMD_SHOWFOUND_CONTROLLER_H_
