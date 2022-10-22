#include "absl/debugging/failure_signal_handler.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "cmd/detect/detection.h"
#include "lib/file/path.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"

ABSL_FLAG(std::string, on_file, "", "On file");
ABSL_FLAG(std::string, off_file, "", "Off file");
ABSL_FLAG(std::string, intermediates_dir, "",
          "Directory for intermediate results");
ABSL_FLAG(bool, show_result, false, "imshow result");

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
  absl::ParseCommandLine(argc, argv);
  absl::InstallFailureSignalHandler(absl::FailureSignalHandlerOptions());

  QCHECK(!absl::GetFlag(FLAGS_on_file).empty()) << "--on_file is required";
  QCHECK(!absl::GetFlag(FLAGS_off_file).empty()) << "--off_file is required";

  LOG(INFO) << "off is " << absl::GetFlag(FLAGS_off_file);
  LOG(INFO) << "on  is " << absl::GetFlag(FLAGS_on_file);

  cv::Mat on_image = cv::imread(absl::GetFlag(FLAGS_on_file));
  cv::Mat off_image = cv::imread(absl::GetFlag(FLAGS_off_file));

  QCHECK_EQ(on_image.rows, off_image.rows) << "size mismatch";
  QCHECK_EQ(on_image.cols, off_image.cols) << "size mismatch";

  cv::Mat blank(on_image.rows, on_image.cols, CV_8U, cv::Scalar::all(0));
  cv::Mat mask;
  cv::bitwise_not(blank, mask);

  std::unique_ptr<DetectResults> results = Detect(off_image, on_image, mask);

  if (!absl::GetFlag(FLAGS_intermediates_dir).empty()) {
    const std::string& dir = absl::GetFlag(FLAGS_intermediates_dir);
    QCHECK_OK(SaveImage(on_image, JoinPath({dir, "on.jpg"})));
    QCHECK_OK(SaveImage(off_image, JoinPath({dir, "off.jpg"})));

    for (const auto& elem : results->intermediates) {
      const std::string& name = elem.first;
      const cv::Mat& image = elem.second;
      QCHECK_OK(SaveImage(image, JoinPath({dir, name + ".jpg"})));
    }
  }

  if (!results->found) {
    return 1;
  }

  if (absl::GetFlag(FLAGS_show_result)) {
    cv::imshow("Result", results->intermediates["marked"]);
    cv::waitKey(0);
  }

  return 0;
}
