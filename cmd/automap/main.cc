#include <memory>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "opencv2/opencv.hpp"

ABSL_FLAG(std::string, camera, "", "URL for camera");

class StreamReader {
 public:
  StreamReader(std::unique_ptr<cv::VideoCapture> stream)
      : stream_(std::move(stream)), should_stop_(false) {}

  ~StreamReader() { stream_.release(); }

  void Reader() {
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

  cv::Mat CurrentFrame() {
    absl::MutexLock lock(&mu_);
    return current_frame_;
  }

  void Stop() {
    absl::MutexLock lock(&mu_);
    should_stop_ = true;
  }

 private:
  absl::Mutex mu_;
  std::unique_ptr<cv::VideoCapture> stream_;
  cv::Mat current_frame_;
  bool should_stop_;
};

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  QCHECK(!absl::GetFlag(FLAGS_camera).empty()) << "--camera is required";

  auto stream = std::make_unique<cv::VideoCapture>();
  QCHECK(stream->open(absl::GetFlag(FLAGS_camera)));

  StreamReader stream_reader(std::move(stream));

  std::thread stream_reader_thread([&] { stream_reader.Reader(); });

  absl::SleepFor(absl::Seconds(5));

  LOG(INFO) << "stopping stream reader";
  stream_reader.Stop();
  stream_reader_thread.join();

  return 0;
}
