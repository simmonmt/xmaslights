#ifndef _CMD_SHOWFOUND_CONTROLLER_VIEW_INTERFACE_H_
#define _CMD_SHOWFOUND_CONTROLLER_VIEW_INTERFACE_H_ 1

class ControllerViewInterface {
 public:
  ~ControllerViewInterface() = default;

  virtual void SetCamera(int camera_num) = 0;
};

#endif  // _CMD_SHOWFOUND_CONTROLLER_VIEW_INTERFACE_H_
