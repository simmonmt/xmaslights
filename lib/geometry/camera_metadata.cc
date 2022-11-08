#include "lib/geometry/camera_metadata.h"

#include "lib/geometry/translation.h"

CameraMetadata CameraMetadata::FromProto(const proto::CameraMetadata& pb) {
  return {
      .distance_from_center = pb.distance_from_center(),
      .fov_h = Radians(pb.fov_h_deg()),
      .fov_v = Radians(pb.fov_v_deg()),
      .res_h = pb.res_h(),
      .res_v = pb.res_v(),
  };
}
