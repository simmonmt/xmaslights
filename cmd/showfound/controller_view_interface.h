#ifndef _CMD_SHOWFOUND_CONTROLLER_VIEW_INTERFACE_H_
#define _CMD_SHOWFOUND_CONTROLLER_VIEW_INTERFACE_H_ 1

#include "opencv2/core/types.hpp"

class ControllerViewInterface {
 public:
  ~ControllerViewInterface() = default;

  virtual void SetCamera(int camera_num) = 0;
  virtual void NextImageMode() = 0;
  virtual void NextSkipMode() = 0;
  virtual void Unfocus() = 0;
  virtual void Focus(int pixel_num) = 0;
  virtual void NextPixel(bool forward) = 0;
  virtual void PrintStatus() = 0;
  virtual bool WritePixels() = 0;
  virtual bool SetPixelLocation(int pixel_num, cv::Point2i location) = 0;
  virtual bool SelectPixel(int pixel_num) = 0;
  virtual void ClearSelectedPixels() = 0;
};

#endif  // _CMD_SHOWFOUND_CONTROLLER_VIEW_INTERFACE_H_
