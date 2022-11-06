#include "cmd/showfound/pixel_writer.h"

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "lib/file/proto.h"
#include "proto/points.pb.h"

FilePixelWriter::FilePixelWriter(const std::string& path) : path_(path) {}

absl::Status FilePixelWriter::WritePixels(
    absl::Span<const ModelPixel> pixels) const {
  std::vector<const ModelPixel*> sorted_pixels(pixels.size());
  for (int i = 0; i < pixels.size(); ++i) {
    sorted_pixels[i] = &pixels[i];
  }

  std::sort(sorted_pixels.begin(), sorted_pixels.end(),
            [](const ModelPixel* a, const ModelPixel* b) {
              return a->num() < b->num();
            });

  proto::PixelRecords out;
  for (const ModelPixel* pixel : sorted_pixels) {
    *out.add_pixel() = pixel->ToProto();
  }

  if (absl::Status status = WriteTextProto(path_, out); !status.ok()) {
    return status;
  }

  LOG(INFO) << absl::StrFormat("wrote %d pixel%s", pixels.size(),
                               (pixels.size() == 1 ? "" : "s"));
  return absl::OkStatus();
}

absl::Status NopPixelWriter::WritePixels(
    absl::Span<const ModelPixel> pixels) const {
  LOG(WARNING) << "ignoring write of " << pixels.size() << " pixels";
  return absl::OkStatus();
}
