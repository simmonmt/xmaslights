#include "cmd/showfound/camera_images.h"

#include <string>

#include "absl/strings/str_format.h"
#include "lib/cv/cv.h"
#include "lib/file/path.h"
#include "opencv2/opencv.hpp"

absl::StatusOr<std::unique_ptr<CameraImages>> CameraImages::Create(
    const std::string& pixel_dir) {
  auto off = CvReadImage(JoinPath({pixel_dir, "off.jpg"}));
  if (!off.ok()) {
    return off.status();
  }

  auto on = CvReadImage(JoinPath({pixel_dir, "on.jpg"}));
  if (!on.ok()) {
    return on.status();
  }

  return std::unique_ptr<CameraImages>(new CameraImages(*off, *on, pixel_dir));
}

std::unique_ptr<CameraImages> CameraImages::CreateWithImages(
    cv::Mat off, cv::Mat on, const std::string& pixel_dir) {
  return std::unique_ptr<CameraImages>(new CameraImages(off, on, pixel_dir));
}

CameraImages::CameraImages(cv::Mat off, cv::Mat on, std::string pixel_dir)
    : off_(off), on_(on), pixel_dir_(pixel_dir) {}

absl::StatusOr<cv::Mat> CameraImages::ReadImage(int pixel_num) {
  const std::string path =
      JoinPath({pixel_dir_, absl::StrFormat("pixel_%03d.jpg", pixel_num)});
  return CvReadImage(path);
}
