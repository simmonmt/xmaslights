#ifndef _LIB_FILE_MERGED_COORDS_H_
#define _LIB_FILE_MERGED_COORDS_H_ 1

#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "absl/status/statusor.h"

struct MergedCoordsRecord {
  int pixel_num;
  std::vector<std::optional<std::tuple<int, int>>> coords;
};

std::ostream& operator<<(std::ostream& os, const MergedCoordsRecord& rec);

absl::StatusOr<std::vector<MergedCoordsRecord>> ReadMergedCoords(
    const std::string& path);

#endif  // _LIB_FILE_MERGED_COORDS_H_
