#ifndef _CMD_SHOWFOUND_CAMERA_IMAGES_H_
#define _CMD_SHOWFOUND_CAMERA_IMAGES_H_ 1

#include <memory>
#include <string>

#include "absl/status/statusor.h"
#include "opencv2/core/mat.hpp"

class CameraImages {
 public:
  ~CameraImages() = default;

  static absl::StatusOr<std::unique_ptr<CameraImages>> Create(
      const std::string& path);
  static std::unique_ptr<CameraImages> CreateWithImages(
      cv::Mat off, cv::Mat on, const std::string& path);

  cv::Mat off() const { return off_; }
  cv::Mat on() const { return on_; }

  absl::StatusOr<cv::Mat> ReadImage(int pixel_num);

 private:
  CameraImages(cv::Mat off, cv::Mat on, std::string pixel_dir);

  cv::Mat off_;
  cv::Mat on_;
  std::string pixel_dir_;
};

#endif  // _CMD_SHOWFOUND_CAMERA_IMAGES_H_
