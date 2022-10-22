#ifndef _CMD_AUTOMAP_STREAM_READER_H_
#define _CMD_AUTOMAP_STREAM_READER_H_ 1

#include "absl/log/log.h"
#include "absl/synchronization/mutex.h"
#include "opencv2/opencv.hpp"

class StreamReader {
 public:
  struct Options {
    bool verbose = false;
  };

  StreamReader(const Options& options)
      : should_stop_(false), options_(options) {}
  virtual ~StreamReader() = default;

  virtual void Read() = 0;

  cv::Mat WaitForFrame();
  cv::Mat CurrentFrame();
  void Stop();

 protected:
  absl::Mutex mu_;
  cv::Mat current_frame_;
  bool should_stop_;
  const Options options_;
};

class VideoCaptureStreamReader : public StreamReader {
 public:
  VideoCaptureStreamReader(const Options& options,
                           std::unique_ptr<cv::VideoCapture> stream)
      : StreamReader(options), stream_(std::move(stream)) {}

  ~VideoCaptureStreamReader() override { stream_.release(); }

  void Read() override;

 private:
  std::unique_ptr<cv::VideoCapture> stream_;
};

#endif  // _CMD_AUTOMAP_STREAM_READER_H_
