#include "cmd/showfound/pixel_writer.h"

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "lib/file/pcd.h"
#include "lib/file/proto.h"
#include "lib/file/xlights.h"
#include "proto/points.pb.h"

FilePixelWriter::FilePixelWriter(const std::string& path) : path_(path) {}

FilePixelWriter& FilePixelWriter::AddPCDOutput(const std::string& path) {
  pcd_path_ = path;
  return *this;
}

FilePixelWriter& FilePixelWriter::AddXLightsOutput(
    const std::string& path, const std::string& model_name) {
  xlights_path_ = path;
  xlights_model_name_ = model_name;
  return *this;
}

absl::Status FilePixelWriter::WritePixels(
    absl::Span<const ModelPixel> pixels) const {
  std::vector<const ModelPixel*> sorted_pixels(pixels.size());
  for (unsigned long i = 0; i < pixels.size(); ++i) {
    sorted_pixels[i] = &pixels[i];
  }

  std::sort(sorted_pixels.begin(), sorted_pixels.end(),
            [](const ModelPixel* a, const ModelPixel* b) {
              return a->num() < b->num();
            });

  if (absl::Status status = WriteAsProto(sorted_pixels); !status.ok()) {
    return status;
  }

  if (!pcd_path_.empty()) {
    if (absl::Status status = WriteAsPCD(sorted_pixels); !status.ok()) {
      return status;
    }
  }

  if (!xlights_path_.empty()) {
    if (absl::Status status = WriteAsXLights(sorted_pixels); !status.ok()) {
      return status;
    }
  }

  return absl::OkStatus();
}

absl::Status FilePixelWriter::WriteAsProto(
    const std::vector<const ModelPixel*>& pixels) const {
  proto::PixelRecords out;
  for (const ModelPixel* pixel : pixels) {
    *out.add_pixel() = pixel->ToProto();
  }

  if (absl::Status status = WriteTextProto(path_, out); !status.ok()) {
    return status;
  }

  LOG(INFO) << absl::StrFormat("wrote %d pixel%s", pixels.size(),
                               (pixels.size() == 1 ? "" : "s"));
  return absl::OkStatus();
}

absl::Status FilePixelWriter::WriteAsPCD(
    const std::vector<const ModelPixel*>& pixels) const {
  auto has_modified_pixel = [](const ModelPixel& pixel) {
    for (const proto::CameraPixelLocation& camera :
         pixel.ToProto().camera_pixel()) {
      if (camera.manually_adjusted()) {
        return true;
      }
    }
    return false;
  };

  std::vector<std::tuple<int, cv::Point3d, cv::viz::Color>> world_pixels;
  for (const ModelPixel* pixel : pixels) {
    if (!pixel->has_world()) {
      continue;
    }

    cv::viz::Color color = cv::viz::Color::green();
    if (has_modified_pixel(*pixel)) {
      color = cv::viz::Color::white();
    } else if (pixel->world_is_derived()) {
      color = cv::viz::Color::orange();
    } else if (pixel->num() < 10) {
      color = cv::viz::Color::yellow();
    }

    world_pixels.emplace_back(pixel->num(), pixel->world(), color);
  }

  return WritePCD(world_pixels, pcd_path_);
}

absl::Status FilePixelWriter::WriteAsXLights(
    const std::vector<const ModelPixel*>& pixels) const {
  std::vector<std::pair<int, cv::Point3d>> points;
  for (const ModelPixel* pixel : pixels) {
    if (pixel->has_world()) {
      points.push_back(std::make_pair(pixel->num(), pixel->world()));
    }
  }

  XLightsModelCreator model_creator(xlights_model_name_);
  std::unique_ptr<XLightsModel> model = model_creator.CreateModel(points);
  return WriteXLightsModel(*model, xlights_path_);
}

absl::Status NopPixelWriter::WritePixels(
    absl::Span<const ModelPixel> pixels) const {
  LOG(WARNING) << "ignoring write of " << pixels.size() << " pixels";
  return absl::OkStatus();
}
