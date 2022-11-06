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

  absl::Status WritePixels(absl::Span<const ModelPixel> pixels) const override;

 private:
  const std::string path_;

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
