#ifndef _CMD_AUTOMAP_STREAM_READER_H_
#define _CMD_AUTOMAP_STREAM_READER_H_ 1

#include "absl/log/log.h"
#include "absl/synchronization/mutex.h"
#include "opencv2/opencv.hpp"

class StreamReader {
 public:
  StreamReader() : should_stop_(false) {}
  virtual ~StreamReader() = default;

  virtual void Read() = 0;

  cv::Mat WaitForFrame();
  cv::Mat CurrentFrame();
  void Stop();

 protected:
  absl::Mutex mu_;
  cv::Mat current_frame_;
  bool should_stop_;
};

class VideoCaptureStreamReader : public StreamReader {
 public:
  VideoCaptureStreamReader(std::unique_ptr<cv::VideoCapture> stream)
      : stream_(std::move(stream)) {}

  ~VideoCaptureStreamReader() override { stream_.release(); }

  void Read() override;

 private:
  std::unique_ptr<cv::VideoCapture> stream_;
};

#endif  // _CMD_AUTOMAP_STREAM_READER_H_
