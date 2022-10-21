#ifndef _CMD_AUTOMAP_DDP_H_
#define _CMD_AUTOMAP_DDP_H_ 1

#include <netdb.h>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/time/time.h"

constexpr int kDefaultDDPPort = 4048;
constexpr absl::Duration kDDPSettleTime = absl::Milliseconds(500);

class DDPConn {
 public:
  ~DDPConn() { close(sock_); }

  struct Options {
    int num_pixels;
    int max_chans_per_packet = 1440;
  };

  static absl::StatusOr<std::unique_ptr<DDPConn>> Create(
      const std::string& hostname, int port, const Options& options);

  absl::Status SetAll(int color);
  absl::Status OnlyOne(int idx, int color);

 private:
  absl::Status SetAll(const std::vector<char>& chans);

  int GetSeq();

  std::vector<unsigned char> MakePacket(absl::Span<const char> chans, int off,
                                        bool last);
  absl::Status SendPacket(absl::Span<const unsigned char> packet);

  DDPConn(int sock, std::unique_ptr<struct sockaddr_in> addr,
          const Options& options)
      : sock_(sock), addr_(std::move(addr)), options_(options), seq_(1) {}

  int sock_;
  std::unique_ptr<struct sockaddr_in> addr_;
  const Options options_;
  int seq_;
};

#endif  // _CMD_AUTOMAP_DDP_H_
