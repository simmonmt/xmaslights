#include <memory>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/time/clock.h"
#include "cmd/automap/ddp.h"
#include "cmd/automap/net.h"
#include "cmd/automap/stream_reader.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

ABSL_FLAG(std::string, camera, "", "URL for camera");
ABSL_FLAG(std::string, controller, "", "host:port for DDP controller");
ABSL_FLAG(int, num_pixels, -1, "Number of pixels on controller");
ABSL_FLAG(int, start_pixel, 0, "Pixel to start with");
ABSL_FLAG(int, end_pixel, -1,
          "Pixel to end with (inclusive); defaults to num_pixels-1");

int main(int argc, char** argv) {
  // absl::InitializeLog();
  absl::ParseCommandLine(argc, argv);

  QCHECK(!absl::GetFlag(FLAGS_camera).empty()) << "--camera is required";
  QCHECK(!absl::GetFlag(FLAGS_controller).empty())
      << "--controller is required";

  auto [host, port] =
      ParseHostPort(absl::GetFlag(FLAGS_controller), kDefaultDDPPort);
  QCHECK(!host.empty()) << "Invalid controller host:port";

  QCHECK_NE(absl::GetFlag(FLAGS_num_pixels), -1) << "--num_pixels is required";
  const int num_pixels = absl::GetFlag(FLAGS_num_pixels);
  const int start_pixel = absl::GetFlag(FLAGS_start_pixel);
  const int end_pixel = [&](int end) {
    return end == -1 ? num_pixels - 1 : end;
  }(absl::GetFlag(FLAGS_end_pixel));

  QCHECK_GT(num_pixels, 0) << "invalid number of pixels";
  QCHECK(start_pixel >= 0 && start_pixel <= num_pixels - 1)
      << "invalid start_pixel";
  QCHECK(end_pixel >= start_pixel && end_pixel <= num_pixels - 1)
      << "invalid end_pixel";
  LOG(INFO) << "num_pixels: " << num_pixels << " scanning [" << start_pixel
            << "," << end_pixel << "]";

  auto stream = std::make_unique<cv::VideoCapture>();
  QCHECK(stream->open(absl::GetFlag(FLAGS_camera)));

  StreamReader stream_reader(std::move(stream));
  std::thread stream_reader_thread([&] { stream_reader.Reader(); });

  cv::Mat frame = stream_reader.WaitForFrame();
  LOG(INFO) << "shape: x=" << frame.cols << " y=" << frame.rows;

  static constexpr char kStreamWindowName[] = "stream";

  cv::namedWindow(kStreamWindowName, cv::WINDOW_AUTOSIZE);
  while (true) {
    cv::Mat image = stream_reader.CurrentFrame();
    cv::imshow(kStreamWindowName, image);
    cv::waitKey(100);
  }

  return 0;
}
