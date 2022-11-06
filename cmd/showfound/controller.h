#ifndef _CMD_SHOWFOUND_CONTROLLER_H_
#define _CMD_SHOWFOUND_CONTROLLER_H_ 1

#include "cmd/showfound/controller_view_interface.h"
#include "cmd/showfound/model.h"
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
  };

  PixelController(Args args);

  void SetCamera(int camera_num) override;
  void NextImageMode() override;
  void Unfocus() override;
  void Focus(int pixel_num) override;
  void NextPixel(bool forward) override;
  void PrintStatus() override;
  bool WritePixels() override;
  bool SetPixelLocation(int pixel_num, cv::Point2i location) override;

 private:
  enum ImageMode {
    IMAGE_ALL_ON,
    IMAGE_ALL_OFF,
    IMAGE_FOCUS_ON,
    IMAGE_LAST,
  };

  std::unique_ptr<ViewPixel> ModelToViewPixel(const ModelPixel& model_pixel,
                                              int camera_num);
  void SetImageMode(ImageMode mode);
  cv::Mat ViewBackgroundImage();

  bool IsValidCameraNum(int camera_num);

  void UpdatePixel(int pixel_num);

  PixelModel& model_;
  PixelView& view_;

  int camera_num_;
  int max_camera_num_;
  int focus_pixel_num_;
  int min_pixel_num_, max_pixel_num_;
  ImageMode image_mode_;

  std::unique_ptr<std::vector<std::unique_ptr<ViewPixel>>> camera_pixels_;
};

#endif  // _CMD_SHOWFOUND_CONTROLLER_H_
