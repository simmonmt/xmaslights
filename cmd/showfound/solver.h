#ifndef _CMD_SHOWFOUND_SOLVER_H_
#define _CMD_SHOWFOUND_SOLVER_H_ 1

#include <optional>

#include "cmd/showfound/model.h"
#include "lib/geometry/camera_metadata.h"
#include "opencv2/core/types.hpp"

class PixelSolver {
 public:
  PixelSolver(const PixelModel& model, const CameraMetadata& metadata);
  ~PixelSolver() = default;

  cv::Point3d CalculateWorldLocation(const ModelPixel& pixel);

  cv::Point3d SynthesizePixelLocation(int camera_number,
                                      cv::Point2i camera_coord,
                                      const int refs[3]);

 private:
  const PixelModel& model_;
  const CameraMetadata metadata_;
};

#endif  // _CMD_SHOWFOUND_SOLVER_H_
