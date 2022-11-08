#ifndef _LIB_GEOMETRY_CAMERA_METADATA_H_
#define _LIB_GEOMETRY_CAMERA_METADATA_H_ 1

#include "proto/camera_metadata.pb.h"

struct CameraMetadata {
  static CameraMetadata FromProto(const proto::CameraMetadata& pb);

  double distance_from_center = 0;  // units
  double fov_h = 0;                 // radians
  double fov_v = 0;                 // radians
  int res_h = 0;
  int res_v = 0;
};

#endif  // _LIB_GEOMETRY_CAMERA_METADATA_H_
