#include "cmd/showfound/model_pixel.h"

ModelPixel::ModelPixel(const proto::PixelRecord& pixel) : pixel_(pixel) {}

ModelPixel::ModelPixel(int num, std::vector<std::optional<cv::Point2i>> cameras,
                       std::optional<cv::Point3d> world) {
  pixel_.set_pixel_number(num);

  for (int i = 0; i < cameras.size(); ++i) {
    const int camera_number = i + 1;
    const std::optional<cv::Point2i>& camera = cameras[i];
    if (!camera.has_value()) {
      continue;
    }

    proto::CameraPixelLocation* camera_proto = pixel_.add_camera_pixel();
    camera_proto->set_camera_number(camera_number);

    proto::Point2i* location_proto = camera_proto->mutable_pixel_location();
    location_proto->set_x(camera->x);
    location_proto->set_y(camera->y);
  }

  if (world.has_value()) {
    proto::Point3d* location_proto =
        pixel_.mutable_world_pixel()->mutable_pixel_location();
    location_proto->set_x(world->x);
    location_proto->set_y(world->y);
    location_proto->set_z(world->z);
  }
}
