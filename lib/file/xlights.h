#ifndef _LIB_FILE_XLIGHTS_H_
#define _LIB_FILE_XLIGHTS_H_ 1

// Creates an XLights Model file from a set of points.
//
// XLights models assume all pixels can be located at an integer x,y,z location,
// with x,y,z all zero or positive. Furthermore, these models specify the value
// (the pixel number) at every possible location (rather than simply being a
// list of points).
//
//   Goal #1: Keep the model size as small as possible, with size being the
//   number of possible locations (i.e. minimize (max_x+1)*(max_y+1)*(max_z+1)).
//
// Our pixel world locations are arbitrary (floating point) locations in 3D
// space, and thus must be mapped to the space XLights expects. There are two
// steps to this process: translation and scaling.
//
// Translation is the movement of all locations such that model min_x, min_y,
// and min_z are zero and model max_x, max_y, and max_z are positive (or zero).
//
// Scaling is the process of determining the range of pixel coordinate values
// that map to a single x,y,z model location. We use the same scaling factor for
// all three axes. A scaling factor of 1 means that pixel x values [0,1) map to
// model x value 0, while 2 maps [0,2) to model x value 0. Collisions result
// when two pixels are mapped to the same model location.
//
//   Goal #2: Minimize the number of collisions.
//
// The two goals are in tension. It is typically not possible to eliminate
// collisions without creating a model that's far too large for XLights to
// handle. We resolve this tension by not letting the model size get too far
// beyond a given size.

#include <map>
#include <set>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/cord.h"
#include "absl/types/span.h"
#include "opencv2/core/types.hpp"
#include "opencv2/viz/types.hpp"

// The XLights representation of a set of points with a scaling factor
// applied. This representation keeps the points sparsely represented
// in a map to make it easier to construct which in turn makes it
// easier to experiment with different scaling factors.
struct XLightsModel {
  std::string name;

  // The scaling factor used to create this model instance.
  double scaling_factor;

  // Scaled and translated pixel locations in model space. The map
  // value is the zero-indexed pixel number. If any collisions occur
  // as a result of scaling, the pixel with the lowest number is
  // retained.
  std::map<std::tuple<int, int, int>, int> points;

  // All collisions that occurred with this scaling factor
  std::set<int> collisions;

  // Per-axis model sizes including the zero coordinate. xsize, for
  // example, is max_x+1.
  int xsize, ysize, zsize;
};

// Creates an XLightsModel with an automatically-chosen scaling factor.
class XLightsModelCreator {
 public:
  XLightsModelCreator(const std::string& model_name);

  // Determines where the scaling factor search will begin.
  void set_initial_scaling_factor(double initial_scaling_factor) {
    initial_scaling_factor_ = initial_scaling_factor;
  }

  // The scaling factor search will end when there are no collisions
  // or the model size exceeds the stop size.
  void set_stop_size(int stop_size) { stop_size_ = stop_size; }

  // Create a model with a scaling factor that satisfies the criteria
  // described above.
  std::unique_ptr<XLightsModel> CreateModel(
      const absl::Span<const std::pair<int, cv::Point3d>>& points);

 private:
  std::unique_ptr<XLightsModel> CreateModelWithScalingFactor(
      const absl::Span<const std::pair<int, cv::Point3d>>& points,
      double scaling_factor);

  const std::string model_name_;
  double initial_scaling_factor_;
  int stop_size_;
};

// Writes a model to disk in the XLights model format.
absl::Status WriteXLightsModel(const XLightsModel& model,
                               const std::string& path);

#endif  // _LIB_FILE_XLIGHTS_H_
