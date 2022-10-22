#include <sys/stat.h>
#include <memory>
#include <thread>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/time/clock.h"
#include "cmd/automap/ddp.h"
#include "cmd/automap/net.h"
#include "cmd/automap/stream_reader.h"
#include "lib/file/path.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

ABSL_FLAG(std::string, camera, "", "URL for camera");
ABSL_FLAG(std::string, controller, "", "host:port for DDP controller");
ABSL_FLAG(int, num_pixels, -1, "Number of pixels on controller");
ABSL_FLAG(int, start_pixel, 0, "Pixel to start with");
ABSL_FLAG(int, end_pixel, -1,
          "Pixel to end with (inclusive); defaults to num_pixels-1");
ABSL_FLAG(std::string, outdir, "", "Output directory for images");
ABSL_FLAG(bool, verbose, false, "Verbose mode");
ABSL_FLAG(absl::Duration, ddp_settle_time, absl::Milliseconds(500),
          "DDP settle time");
ABSL_FLAG(bool, display, true, "Display in-progress results");

namespace {

absl::Status SaveImage(cv::Mat mat, const std::string& filename) {
  if (!cv::imwrite(filename, mat)) {
    return absl::UnknownError(
        absl::StrCat("failed to write image to ", filename));
  }
  return absl::OkStatus();
}

}  // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("takes pictures of pixels");
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  QCHECK(!absl::GetFlag(FLAGS_camera).empty()) << "--camera is required";
  QCHECK(!absl::GetFlag(FLAGS_controller).empty())
      << "--controller is required";

  QCHECK(!absl::GetFlag(FLAGS_outdir).empty()) << "--outdir is required";
  const std::string outdir = absl::GetFlag(FLAGS_outdir);

  std::string hostname;
  int port;
  std::tie(hostname, port) =
      ParseHostPort(absl::GetFlag(FLAGS_controller), kDefaultDDPPort);
  QCHECK(!hostname.empty()) << "Invalid controller host:port";

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

  const absl::Duration settle_time = absl::GetFlag(FLAGS_ddp_settle_time);

  if (mkdir(absl::GetFlag(FLAGS_outdir).c_str(), 0777) < 0 && errno != EEXIST) {
    QCHECK(false) << "Failed to make outdir: " << strerror(errno);
  }

  std::unique_ptr<DDPConn> ddp_conn = [&]() {
    auto statusor = DDPConn::Create(
        hostname, port,
        {.num_pixels = num_pixels, .verbose = absl::GetFlag(FLAGS_verbose)});
    QCHECK_OK(statusor);
    return std::move(*statusor);
  }();

  auto stream = std::make_unique<cv::VideoCapture>();
  QCHECK(stream->open(absl::GetFlag(FLAGS_camera)));

  VideoCaptureStreamReader stream_reader(
      {.verbose = absl::GetFlag(FLAGS_verbose)}, std::move(stream));
  std::thread stream_reader_thread([&] { stream_reader.Read(); });

  cv::Mat frame = stream_reader.WaitForFrame();
  LOG(INFO) << "shape: x=" << frame.cols << " y=" << frame.rows;

  static constexpr char kStreamWindowName[] = "stream";

  if (absl::GetFlag(FLAGS_display)) {
    cv::namedWindow(kStreamWindowName, cv::WINDOW_AUTOSIZE);
  }

  QCHECK_OK(ddp_conn->SetAll(0));
  absl::SleepFor(settle_time);
  QCHECK_OK(
      SaveImage(stream_reader.CurrentFrame(), JoinPath({outdir, "off.jpg"})));

  QCHECK_OK(ddp_conn->SetAll(0xffffff));
  absl::SleepFor(settle_time);
  QCHECK_OK(
      SaveImage(stream_reader.CurrentFrame(), JoinPath({outdir, "on.jpg"})));

  for (int i = start_pixel; i <= end_pixel; ++i) {
    LOG(INFO) << "pixel " << i;
    QCHECK_OK(ddp_conn->OnlyOne(i, 0xff'ff'ff));
    absl::SleepFor(settle_time);

    cv::Mat image = stream_reader.CurrentFrame();
    if (absl::GetFlag(FLAGS_display)) {
      cv::imshow(kStreamWindowName, image);
      cv::waitKey(1);
    }
    QCHECK_OK(SaveImage(stream_reader.CurrentFrame(),
                        JoinPath({outdir, absl::StrCat("pixel_", i, ".jpg")})));
  }

  stream_reader.Stop();
  stream_reader_thread.join();

  return 0;
}
