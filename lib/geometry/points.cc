#include "lib/geometry/points.h"

#include "absl/log/log.h"
#include "absl/types/span.h"

std::ostream& operator<<(std::ostream& os, const XYPos& pos) {
  return os << "[" << pos.x << "," << pos.y << "]";
}

std::ostream& operator<<(std::ostream& os, const XYZPos& pos) {
  return os << "[" << pos.x << "," << pos.y << "," << pos.z << "]";
}

void PointsIndex::Add(const XYZPos& pos, double key) {
  index_.emplace(key, pos);
}

std::vector<XYZPos> PointsIndex::Near(double where, double within) {
  std::vector<XYZPos> out;
  double max_upper = where + within;
  for (auto iter = index_.lower_bound(where - within); iter != index_.end();
       ++iter) {
    if (iter->first > max_upper) {
      break;
    }
    out.push_back(iter->second);
  }
  return out;
}
