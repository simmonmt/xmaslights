#include "lib/file/coords.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "lib/file/readers.h"
#include "opencv2/core/types.hpp"

std::ostream& operator<<(std::ostream& os, const CoordsRecord& rec) {
  os << absl::StreamFormat("%03d", rec.pixel_num);

  for (const auto& coord : rec.camera_coords) {
    if (coord.has_value()) {
      os << absl::StreamFormat(" %d,%d", coord->x, coord->y);
    } else {
      os << " -";
    }
  }

  if (rec.world_coord.has_value()) {
    const cv::Point3d& c = *rec.world_coord;
    os << " " << absl::StreamFormat(" %f,%f,%f", c.x, c.y, c.z);
  }

  return os;
}

namespace {

bool ParseXYCoord(const std::string& str, cv::Point2i* coord) {
  std::vector<std::string> parts = absl::StrSplit(str, absl::MaxSplits(',', 1));
  if (parts.size() != 2) {
    return false;
  }

  return absl::SimpleAtoi(parts[0], &coord->x) &&
         absl::SimpleAtoi(parts[1], &coord->y);
}

bool ParseXYZCoord(const std::string& str, cv::Point3d* coord) {
  std::vector<std::string> parts = absl::StrSplit(str, absl::MaxSplits(',', 2));
  if (parts.size() != 3) {
    return false;
  }

  return absl::SimpleAtod(parts[0], &coord->x) &&
         absl::SimpleAtod(parts[1], &coord->y) &&
         absl::SimpleAtod(parts[2], &coord->z);
}

absl::StatusOr<std::unordered_map<int, std::vector<std::optional<cv::Point2i>>>>
ReadCameraCoords(const std::string& path) {
  auto make_error = [&](int lineno, const std::string& suffix) {
    return MakeParseError(path, lineno, suffix);
  };

  std::unordered_map<int, std::vector<std::optional<cv::Point2i>>>
      coords_by_pixel;
  absl::Status status = ReadFields(
      path, " ", 3, [&](int lineno, absl::Span<const std::string> parts) {
        unsigned int pixel_num;
        if (!absl::SimpleAtoi(parts[0], &pixel_num) && pixel_num > 65536) {
          return make_error(lineno, "pixel number invalid");
        }

        std::vector<std::optional<cv::Point2i>> coords;
        for (int j = 1; j < parts.size(); ++j) {
          const std::string& coord = parts[j];
          cv::Point2i point;
          if (coord == "-") {
            coords.push_back(std::nullopt);
          } else if (ParseXYCoord(coord, &point)) {
            coords.push_back(point);
          } else {
            return make_error(lineno,
                              absl::StrFormat("coordinate %d invalid", j));
          }
        }

        coords_by_pixel.emplace(std::move(pixel_num), std::move(coords));
        return absl::OkStatus();
      });

  if (!status.ok()) {
    return status;
  }

  return coords_by_pixel;
}

absl::StatusOr<std::unordered_map<int, cv::Point3d>> ReadWorldCoords(
    const std::string& path) {
  auto make_error = [&](int lineno, const std::string& suffix) {
    return MakeParseError(path, lineno, suffix);
  };

  std::unordered_map<int, cv::Point3d> coords_by_pixel;
  absl::Status status = ReadFields(
      path, " ", 2, [&](int lineno, absl::Span<const std::string> parts) {
        unsigned int pixel_num;
        if (!absl::SimpleAtoi(parts[0], &pixel_num) && pixel_num > 65536) {
          return make_error(lineno, "pixel number invalid");
        }

        cv::Point3d point;
        if (!ParseXYZCoord(parts[1], &point)) {
          return make_error(lineno, "world coordinate invalid");
        }

        coords_by_pixel.emplace(pixel_num, point);
        return absl::OkStatus();
      });

  if (!status.ok()) {
    return status;
  }

  return coords_by_pixel;
}

}  // namespace

absl::StatusOr<std::vector<CoordsRecord>> ReadCoords(
    std::optional<std::string> camera_coords_path,
    std::optional<std::string> world_coords_path) {
  if (!camera_coords_path.has_value() && !world_coords_path.has_value()) {
    return absl::InvalidArgumentError("no paths specified");
  }

  std::unordered_map<int, std::vector<std::optional<cv::Point2i>>>
      camera_coords;
  if (camera_coords_path.has_value()) {
    auto result = ReadCameraCoords(*camera_coords_path);
    if (!result.ok()) {
      return result.status();
    }
    camera_coords = std::move(*result);
  }

  std::unordered_map<int, cv::Point3d> world_coords;
  if (world_coords_path.has_value()) {
    auto result = ReadWorldCoords(*world_coords_path);
    if (!result.ok()) {
      return result.status();
    }
    world_coords = std::move(*result);
  }

  std::set<int> known_pixels;
  for (const auto& iter : camera_coords) {
    known_pixels.insert(iter.first);
  }
  for (const auto& iter : world_coords) {
    known_pixels.insert(iter.first);
  }

  std::vector<CoordsRecord> out(known_pixels.size());
  int i = 0;
  for (const int pixel_num : known_pixels) {
    CoordsRecord& rec = out[i++];
    rec.pixel_num = pixel_num;

    if (auto iter = camera_coords.find(pixel_num);
        iter != camera_coords.end()) {
      rec.camera_coords = std::move(iter->second);
    }
    if (auto iter = world_coords.find(pixel_num); iter != world_coords.end()) {
      rec.world_coord = iter->second;
    }
  }

  return out;
}
