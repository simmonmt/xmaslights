#ifndef _CMD_SHOWFOUND_PIXEL_WRITER_H_
#define _CMD_SHOWFOUND_PIXEL_WRITER_H_ 1

#include <string>

#include "absl/status/status.h"
#include "absl/types/span.h"
#include "cmd/showfound/model_pixel.h"
#include "lib/base/base.h"

class PixelWriter {
 public:
  virtual ~PixelWriter() = default;

  virtual absl::Status WritePixels(
      absl::Span<const ModelPixel> pixels) const = 0;
};

class FilePixelWriter : public PixelWriter {
 public:
  FilePixelWriter(const std::string& path);
  ~FilePixelWriter() override = default;

  FilePixelWriter& AddPCDOutput(const std::string& path);
  FilePixelWriter& AddXLightsOutput(const std::string& path,
                                    const std::string& model_name);

  absl::Status WritePixels(absl::Span<const ModelPixel> pixels) const override;

 private:
  absl::Status WriteAsProto(const std::vector<const ModelPixel*>& pixels) const;
  absl::Status WriteAsPCD(const std::vector<const ModelPixel*>& pixels) const;
  absl::Status WriteAsXLights(
      const std::vector<const ModelPixel*>& pixels) const;

  const std::string path_;
  std::string pcd_path_;
  std::string xlights_path_;
  std::string xlights_model_name_;

  DISALLOW_COPY_AND_ASSIGN(FilePixelWriter);
};

class NopPixelWriter : public PixelWriter {
 public:
  NopPixelWriter() = default;
  ;
  ~NopPixelWriter() override = default;

  absl::Status WritePixels(absl::Span<const ModelPixel> pixels) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NopPixelWriter);
};

#endif  // _CMD_SHOWFOUND_PIXEL_WRITER_H_
