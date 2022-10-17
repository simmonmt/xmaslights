#include "cmd/automap/stream_reader.h"

#include "absl/log/log.h"
#include "absl/synchronization/mutex.h"
#include "opencv2/opencv.hpp"

void StreamReader::Reader() {
  for (;;) {
    {
      absl::MutexLock lock(&mu_);
      if (should_stop_) {
        return;
      }
    }

    if (!stream_->grab()) {
      absl::MutexLock lock(&mu_);
      mu_.AwaitWithTimeout(absl::Condition(&should_stop_),
                           absl::Milliseconds(10));
      continue;
    }

    cv::Mat frame;
    stream_->retrieve(frame);
    if (frame.empty()) {
      LOG(WARNING) << "empty frame read";
      continue;
    }

    LOG(INFO) << "read frame";

    absl::MutexLock lock(&mu_);
    current_frame_ = frame;
  }
}

cv::Mat StreamReader::WaitForFrame() {
  auto has_frame = [this]() {
    mu_.AssertReaderHeld();
    return !current_frame_.empty();
  };

  absl::MutexLock lock(&mu_);
  mu_.Await(absl::Condition(&has_frame));
  return current_frame_;
}

cv::Mat StreamReader::CurrentFrame() {
  absl::MutexLock lock(&mu_);
  return current_frame_;
}

void StreamReader::Stop() {
  absl::MutexLock lock(&mu_);
  should_stop_ = true;
}
