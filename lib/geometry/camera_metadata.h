#ifndef _LIB_GEOMETRY_CAMERA_METADATA_H_
#define _LIB_GEOMETRY_CAMERA_METADATA_H_ 1

struct CameraMetadata {
  double distance_from_center;  // units
  double fov_h;                 // radians
  double fov_v;                 // radians
  int res_h;
  int res_v;
};

#endif  // _LIB_GEOMETRY_CAMERA_METADATA_H_
