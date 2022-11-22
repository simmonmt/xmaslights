#include "cmd/showfound/model_pixel.h"

#include "proto/points.pb.h"

ModelPixel::ModelPixel(const proto::PixelRecord& pixel) : pixel_(pixel) {}

ModelPixel::ModelPixel(int num, std::vector<std::optional<cv::Point2i>> cameras,
                       std::optional<cv::Point3d> world) {
  pixel_.set_pixel_number(num);

  for (unsigned long i = 0; i < cameras.size(); ++i) {
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

ModelPixelBuilder::ModelPixelBuilder(const ModelPixel& orig)
    : proto_(orig.ToProto()) {}

namespace {

proto::CameraPixelLocation MakeCameraPixelLocation(int camera_num,
                                                   cv::Point2i location,
                                                   bool manual_update) {
  proto::CameraPixelLocation cpl;
  cpl.set_camera_number(camera_num);
  cpl.mutable_pixel_location()->set_x(location.x);
  cpl.mutable_pixel_location()->set_y(location.y);
  if (manual_update) {
    cpl.set_manually_adjusted(manual_update);
  }
  return cpl;
}

}  // namespace

ModelPixelBuilder& ModelPixelBuilder::SetCameraLocation(int camera_num,
                                                        cv::Point2i location,
                                                        bool manual_update) {
  bool updated = false;
  for (proto::CameraPixelLocation& camera : *proto_.mutable_camera_pixel()) {
    if (camera.camera_number() != camera_num) {
      continue;
    }
    updated = true;
    camera = MakeCameraPixelLocation(camera_num, location, manual_update);
  }

  if (!updated) {
    *proto_.add_camera_pixel() =
        MakeCameraPixelLocation(camera_num, location, manual_update);
  }

  std::sort(proto_.mutable_camera_pixel()->begin(),
            proto_.mutable_camera_pixel()->end(),
            [](const proto::CameraPixelLocation& a,
               const proto::CameraPixelLocation& b) {
              return a.camera_number() < b.camera_number();
            });

  return *this;
}

ModelPixelBuilder& ModelPixelBuilder::ClearCameraLocation(int camera_num) {
  for (auto iter = proto_.camera_pixel().begin();
       iter != proto_.camera_pixel().end(); ++iter) {
    const proto::CameraPixelLocation& camera = *iter;
    if (camera.camera_number() == camera_num) {
      proto_.mutable_camera_pixel()->erase(iter);
      break;
    }
  }

  return *this;
}

ModelPixelBuilder& ModelPixelBuilder::SetWorldLocation(
    cv::Point3d location, std::optional<std::set<int>> synthesis_source) {
  proto::WorldPixelLocation* world = proto_.mutable_world_pixel();
  world->Clear();

  proto::Point3d* point = world->mutable_pixel_location();
  point->set_x(location.x);
  point->set_y(location.y);
  point->set_z(location.z);

  if (synthesis_source.has_value()) {
    proto::WorldPixelDerivation* derivation = world->mutable_derivation();
    for (const int source : *synthesis_source) {
      derivation->add_derived_from(source);
    }
  }

  return *this;
}

ModelPixelBuilder& ModelPixelBuilder::ClearWorldLocation() {
  proto_.clear_world_pixel();
  return *this;
}

ModelPixel ModelPixelBuilder::Build() { return ModelPixel(proto_); }
