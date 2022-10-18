#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "ddp.h"
#include "net.h"

ABSL_FLAG(std::string, controller, "", "host:port for DDP controller");
ABSL_FLAG(int, num_pixels, -1, "Number of pixels on controller");
ABSL_FLAG(bool, all_on, false, "All on");
ABSL_FLAG(bool, all_off, false, "All off");
ABSL_FLAG(int, one_on, -1, "Pixel to turn on");
ABSL_FLAG(int, color, 0xff'ff'ff, "color to use");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  QCHECK(!absl::GetFlag(FLAGS_controller).empty())
      << "--controller is required";

  auto [host, port] =
      ParseHostPort(absl::GetFlag(FLAGS_controller), kDefaultDDPPort);
  QCHECK(!host.empty()) << "Invalid controller host:port";

  QCHECK_NE(absl::GetFlag(FLAGS_num_pixels), -1) << "--num_pixels is required";
  const int num_pixels = absl::GetFlag(FLAGS_num_pixels);
  QCHECK_GT(num_pixels, 0) << "invalid number of pixels";

  auto conn = DDPConn::Create(host, port,
                              {.num_pixels = absl::GetFlag(FLAGS_num_pixels),
                               .max_chans_per_packet = 1440});
  QCHECK(conn.ok()) << conn.status().ToString();

  if (absl::GetFlag(FLAGS_all_on)) {
    QCHECK_OK((*conn)->SetAll(absl::GetFlag(FLAGS_color)));
  } else if (absl::GetFlag(FLAGS_all_off)) {
    QCHECK_OK((*conn)->SetAll(0));
  } else if (int idx = absl::GetFlag(FLAGS_one_on); idx > 0) {
    QCHECK_OK((*conn)->OnlyOne(idx, absl::GetFlag(FLAGS_color)));
  } else {
    QCHECK(false) << "no command specified";
  }

  return 0;
}
