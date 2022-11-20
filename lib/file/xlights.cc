#include "lib/file/xlights.h"

#include <fstream>
#include <map>
#include <tuple>

#include "absl/log/log.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"

namespace {

std::string ModelToString(const XLightsModel& model) {
  absl::Cord out;
  for (int z = 0; z < model.zsize; ++z) {
    if (z != 0) {
      out.Append("|");
    }

    for (int y = 0; y < model.ysize; ++y) {
      if (y != 0) {
        out.Append(";");
      }

      std::vector<std::string> line;
      for (int x = 0; x < model.xsize; ++x) {
        if (auto iter = model.points.find(std::make_tuple(x, y, z));
            iter != model.points.end()) {
          line.push_back(absl::StrCat(iter->second + 1));
        } else {
          line.push_back("");
        }
      }

      out.Append(absl::StrJoin(line, ","));
    }
  }

  return std::string(out);
}

}  // namespace

XLightsModelCreator::XLightsModelCreator(const std::string& model_name)
    : model_name_(model_name),
      initial_scaling_factor_(10.0),
      stop_size_(1'000'000) {}

std::unique_ptr<XLightsModel> XLightsModelCreator::CreateModelWithScalingFactor(
    const absl::Span<const std::pair<int, cv::Point3d>>& points,
    double scaling_factor) {
  constexpr double min = -std::numeric_limits<double>::max();
  constexpr double max = std::numeric_limits<double>::max();
  double xmin = max, xmax = min, ymin = max, ymax = min, zmin = max, zmax = min;

  for (const auto& [num, point] : points) {
    xmin = std::min(xmin, point.x);
    xmax = std::max(xmax, point.x);
    ymin = std::min(ymin, point.y);
    ymax = std::max(ymax, point.y);
    zmin = std::min(zmin, point.z);
    zmax = std::max(zmax, point.z);
  }

  int xsz = (xmax - xmin) / scaling_factor + 1;
  int ysz = (ymax - ymin) / scaling_factor + 1;
  int zsz = (zmax - zmin) / scaling_factor + 1;

  // There's no built-in hash implementation for tuple and I'm too lazy to make
  // one.
  std::map<std::tuple<int, int, int>, int> indexed;

  std::set<int> collisions;
  for (const auto& [num, point] : points) {
    auto [iter, inserted] =
        indexed.emplace(std::make_tuple((point.x - xmin) / scaling_factor,
                                        (point.y - ymin) / scaling_factor,
                                        (point.z - zmin) / scaling_factor),
                        num);
    if (!inserted) {
      if (num < iter->second) {
        collisions.insert(iter->second);
        iter->second = num;
      } else {
        collisions.insert(num);
      }
    }
  }

  return std::make_unique<XLightsModel>(XLightsModel{
      .name = model_name_,
      .points = indexed,
      .collisions = collisions,
      .scaling_factor = scaling_factor,
      .xsize = xsz,
      .ysize = ysz,
      .zsize = zsz,
  });
}

std::unique_ptr<XLightsModel> XLightsModelCreator::CreateModel(
    const absl::Span<const std::pair<int, cv::Point3d>>& points) {
  std::vector<std::pair<int, cv::Point3d>> rotated_points;
  for (const auto& [num, point] : points) {
    rotated_points.push_back(
        std::make_pair(num, cv::Point3d{point.x, -point.z, point.y}));
  }

  for (double scaling_factor = initial_scaling_factor_; scaling_factor != 0;
       scaling_factor /= 2.0) {
    std::unique_ptr<XLightsModel> model =
        CreateModelWithScalingFactor(rotated_points, scaling_factor);
    int model_size = model->xsize * model->ysize * model->zsize;

    LOG(INFO) << absl::StrFormat(
        "scaling_factor: %f, #collisions: %d, size: %dx%dx%d=%d",
        model->scaling_factor, model->collisions.size(), model->xsize,
        model->ysize, model->zsize, model_size);
    if (model->collisions.empty() || model_size >= stop_size_) {
      return model;
    }
  }
  LOG(FATAL) << "scaling_factor reached zero";
}

static constexpr char kXMLTemplate[] = R"(<?xml version="1.0" encoding="UTF-8"?>
<custommodel name="%s"
	     parm1="%d"
	     parm2="%d"
	     Depth="%d"
	     StringType="RGB Nodes"
	     Transparency="0"
	     PixelSize="2"
	     ModelBrightness=""
	     Antialias="1"
	     StrandNames=""
	     NodeNames=""
	     CustomModel="%s"
	     SourceVersion="2020.37"  >
</custommodel>
)";

absl::Status WriteXLightsModel(const XLightsModel& model,
                               const std::string& path) {
  std::ofstream out;
  out.open(path);
  if (!out.good()) {
    return absl::UnknownError("failed to open file for writing");
  }

  out << absl::StreamFormat(kXMLTemplate, model.name, model.xsize, model.ysize,
                            model.zsize, ModelToString(model));

  out.close();
  if (!out.good()) {
    return absl::UnknownError("failed to close file");
  }

  return absl::OkStatus();
}
