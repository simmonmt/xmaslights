#ifndef _CMD_AUTOMAP_STREAM_READER_H_
#define _CMD_AUTOMAP_STREAM_READER_H_ 1

#include "absl/log/log.h"
#include "absl/synchronization/mutex.h"
#include "opencv2/opencv.hpp"

class StreamReader {
 public:
  StreamReader(std::unique_ptr<cv::VideoCapture> stream)
      : stream_(std::move(stream)), should_stop_(false) {}

  ~StreamReader() { stream_.release(); }

  void Reader();
  cv::Mat WaitForFrame();
  cv::Mat CurrentFrame();
  void Stop();

 private:
  absl::Mutex mu_;
  std::unique_ptr<cv::VideoCapture> stream_;
  cv::Mat current_frame_;
  bool should_stop_;
};

#endif  // _CMD_AUTOMAP_STREAM_READER_H_
