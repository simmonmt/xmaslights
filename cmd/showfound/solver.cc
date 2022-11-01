#include "cmd/showfound/solver.h"

#include <optional>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "cmd/showfound/model.h"
#include "lib/geometry/camera_metadata.h"
#include "opencv2/core/types.hpp"

namespace {

constexpr double PI_3 = M_PI / 3.0;

std::tuple<double, double, double> FindNormalVector(cv::Point3d refs[3]) {
  const cv::Point3d v1 = refs[0] - refs[1];
  const cv::Point3d v2 = refs[0] - refs[2];

  // n = v1 x v2
  //
  // |  i   j   k  |
  // | v1x v1y v1z |
  // | v2x v2y v2z |
  //
  // n = (v1y*v2z-v1z*v2y, -v1x*v2z-v1z*v2x, v1x*v2y-v1y*v2x)
  return std::make_tuple(v1.y * v2.z - v1.z * v2.y,     // i
                         -(v1.x * v2.z - v1.z * v2.x),  // j
                         v1.x * v2.y - v1.y * v2.x);    // k
}

double FindAngleRad(int pixel, int res, double fov) {
  double res_half = res / 2.0;
  return (fov / 2.0) * (static_cast<double>(pixel - res_half) / res_half);
}

}  // namespace

PixelSolver::PixelSolver(const PixelModel& model,
                         const CameraMetadata& metadata)
    : model_(model), metadata_(metadata) {}

cv::Point3d PixelSolver::SynthesizePixelLocation(int camera_number,
                                                 cv::Point2i camera_coord,
                                                 const int refs[3]) {
  QCHECK(camera_number == 1 || camera_number == 2);

  cv::Point2i ref_camera_coords[3];
  cv::Point3d ref_world_coords[3];
  for (int i = 0; i < 3; ++i) {
    ref_camera_coords[i] = model_.FindPixel(refs[i])->camera;
    ref_world_coords[i] = *(model_.FindPixel(refs[i])->world);
  }

  // First: figure out the world z coordinate.
  //
  // z is different from x and y in that we calculated z by taking the average
  // calculated z positions from two cameras. Doing it this way splits the
  // difference between the camera y offsets.  It's therefore impossible to
  // start with a single camera y coordinate and end up with an accurate world
  // position without a corresponding y coordinate from the other camera
  // influenced by its y offset.
  //
  // Instead we calculate world z via interpolation on the camera's y axis. If
  // the camera y coordinate is halfway between the min and max y camera
  // coordinates for the reference pixels, then the world z must be halfway
  // between the min and max z coordinates for those pixels.
  //
  // Trap: Recall that camera y is zero at the top, increasing as we go down,
  // while world z is zero at the bottom, increasing as we go up. We have to
  // take this into account when applying the interpolation percentage.

  int camera_min_y, camera_max_y;
  double world_min_z, world_max_z;
  camera_min_y = camera_max_y = ref_camera_coords[0].y;
  world_min_z = world_max_z = ref_world_coords[0].z;
  for (int i = 1; i < 3; ++i) {
    camera_min_y = std::min(camera_min_y, ref_camera_coords[i].y);
    camera_max_y = std::max(camera_max_y, ref_camera_coords[i].y);
    world_min_z = std::min(world_min_z, ref_world_coords[i].z);
    world_max_z = std::max(world_max_z, ref_world_coords[i].z);
  }

  const double camera_pct_from_min =  // percentage from the top
      static_cast<double>(camera_coord.y - camera_min_y) /
      (camera_max_y - camera_min_y);
  const double world_z =
      world_min_z +
      (world_max_z - world_min_z) *
          (1.0 - camera_pct_from_min);  // percentage from the bottom

  // Second we calculate the world x/y ray that corresponds to the camera x
  // coordinate. We don't care about the z component because we already know the
  // world z. We calculate the ray using the parametric form (vs using y=mx+b)
  // because we're going to need them in parametric form to intersect with the
  // plane.

  cv::Point3d camera_pos;
  if (camera_number == 1) {
    camera_pos = {metadata_.distance_from_center, 0, 0};
  } else {
    camera_pos = {
        -std::cos(PI_3) * metadata_.distance_from_center,
        std::sin(PI_3) * metadata_.distance_from_center,
        0,
    };
  }

  double xy_angle =
      FindAngleRad(camera_coord.x, metadata_.res_h, metadata_.fov_h);
  // Correct for the different viewing angles. Camera 1 looks along the x axis,
  // while camera 2 is 120deg offset CCW from camera 1.
  if (camera_number == 1) {
    xy_angle = M_PI - xy_angle;
  } else {
    xy_angle = 2.0 * PI_3 - xy_angle;
  }

  // The parametric form is x=x0+ta, y=y0+tb. We use the camera position for
  // (x0,y0), write ta and tb as line_a and line_b, and ignore z as described
  // above.
  double line_a = 1;                   // camera to pixel run
  double line_b = std::tan(xy_angle);  // camera to pixel rise

  // Third find the plane described by the three reference points. The plane
  // equation is: ax + by + cz + d = 0.
  //
  // We'll put it in scalar form: a(x-x0) + b(y-y0) + c(z-z0) = 0
  //                              d = -(a*x0 + b*y0 + c*z0)
  //
  // We need to solve for a, b, c, and d. a, b, and c are the components of the
  // normal vector. x0, y0, and z0 are from a known point. Once we have all
  // those, we can calculate d.

  double plane_a, plane_b, plane_c;
  std::tie(plane_a, plane_b, plane_c) = FindNormalVector(ref_world_coords);

  double plane_d = -(plane_a * ref_world_coords[0].x +  //
                     plane_b * ref_world_coords[0].y +  //
                     plane_c * ref_world_coords[0].z);

  // Fourth: We find the world coordinates. Ideally we'd find the intersection
  // between the ray and the plane, which would give us the world coordinates
  // for the point we identified in the camera's image. We'd substitute the
  // parametric line equations for x, y, and z, solving for t. We'd plug t back
  // into the line equations to get world x, y, and z.
  //
  // Our special world z complicates (simplifies?) matters. We don't have a
  // parametric equation for the z component of the ray, but we have something
  // better -- we already have the world z value. This means we can substitute
  // the parametric equations for x and y into the plane equation, leaving this:
  //
  //         a*x         +        b*y         + c*z + d = 0
  //  a*(cam_x+line_a*t) + b*(cam_y+line_b*t) + c*z + d = 0
  //
  // We know everything but t, so we can solve for t:
  //
  //  a*cam_x+a*line_a*t + b*cam_y+b*line_b*t + c*cam_z+c*line_c*t = -d
  //  a*line_a*t + b*line_b*t + c*line_c*t = -d - a*cam_x - b*cam_y - c*cam_z
  //  (a*line_a + b*line_b + c*line_c)*t   = -d - a*cam_x - b*cam_y - c*cam_z
  //  t = (-d - a*cam_x - b*cam_y - c*cam_z)/(a*line_a + b*line_b + c*line_c)
  //
  // Once we have t we can solve for world x and y.
  double t = (-plane_d - (plane_a * camera_pos.x) -  //
              (plane_b * camera_pos.y) - (plane_c * world_z)) /
             (plane_a * line_a + plane_b * line_b);

  cv::Point3d intersection = {
      camera_pos.x + line_a * t,  //
      camera_pos.y + line_b * t,  //
      world_z,
  };

  return intersection;
}
