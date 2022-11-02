#ifndef _CMD_SHOWFOUND_CONTROLLER_H_
#define _CMD_SHOWFOUND_CONTROLLER_H_ 1

#include "cmd/showfound/model.h"
#include "cmd/showfound/view.h"

class PixelController {
 public:
  PixelController(int camera_num, PixelModel& model, PixelView& view);

  void SetCamera(int camera_num);

 private:
  int camera_num_;
  PixelModel& model_;
  PixelView& view_;
};

#endif  // _CMD_SHOWFOUND_CONTROLLER_H_
