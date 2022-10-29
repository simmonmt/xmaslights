#include "lib/file/merged_coords.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

std::ostream& operator<<(std::ostream& os, const MergedCoordsRecord& rec) {
  std::vector<std::string> coords;
  for (const auto& coord : rec.coords) {
    if (coord.has_value()) {
      coords.push_back(
          absl::StrFormat("%d,%d", std::get<0>(*coord), std::get<1>(*coord)));
    } else {
      coords.push_back("-");
    }
  }

  return os << absl::StreamFormat("%03d %s", rec.pixel_num,
                                  absl::StrJoin(coords, " "));
}

namespace {

bool ParseCoord(const std::string& str, int* x, int* y) {
  std::vector<std::string> parts = absl::StrSplit(str, absl::MaxSplits(',', 1));

  return absl::SimpleAtoi(parts[0], x) && absl::SimpleAtoi(parts[1], y);
}

}  // namespace

absl::StatusOr<std::vector<MergedCoordsRecord>> ReadMergedCoords(
    const std::string& path) {
  std::ifstream infile(path);

  auto error_message = [](int lineno, const std::string& suffix) {
    return absl::StrCat("line ", lineno, ": ", suffix);
  };

  std::vector<MergedCoordsRecord> recs;
  std::string line;
  for (int i = 1; std::getline(infile, line); ++i) {
    std::vector<std::string> parts = absl::StrSplit(line, " ");
    if (parts.size() != 3) {
      return absl::InvalidArgumentError(error_message(i, "too many coords"));
    }

    unsigned int pixel_num;
    if (!absl::SimpleAtoi(parts[0], &pixel_num) && pixel_num > 65536) {
      return absl::InvalidArgumentError(
          error_message(i, "pixel number invalid"));
    }

    MergedCoordsRecord rec;
    rec.pixel_num = pixel_num;

    for (int j = 1; j < parts.size(); ++j) {
      const std::string& coord = parts[j];
      int x, y;
      if (coord == "-") {
        rec.coords.push_back(std::nullopt);
      } else if (ParseCoord(coord, &x, &y)) {
        rec.coords.emplace_back(std::make_tuple(x, y));
      } else {
        return absl::InvalidArgumentError(
            error_message(i, absl::StrFormat("coordinate %d invalid", j)));
      }
    }

    recs.push_back(rec);
  }

  return recs;
}
