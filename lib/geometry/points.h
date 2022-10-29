#ifndef _LIB_GEOMETRY_POINTS_H_
#define _LIB_GEOMETRY_POINTS_H_ 1

#include <iostream>
#include <map>
#include <vector>

#include "absl/types/span.h"

struct XYPos {
  double x;
  double y;
};

struct XYZPos {
  double x;
  double y;
  double z;
};

std::ostream& operator<<(std::ostream& os, const XYPos& pos);

class PointsIndex {
 public:
  PointsIndex() = default;
  ~PointsIndex() = default;

  void Add(const XYZPos& pos, double key);

  std::vector<XYZPos> Near(double where, double within);

 private:
  std::multimap<double, XYZPos> index_;
};

std::ostream& operator<<(std::ostream& os, const XYZPos& pos);

#endif  // _LIB_GEOMETRY_POINTS_H_
