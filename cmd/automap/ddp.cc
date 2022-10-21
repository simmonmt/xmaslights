#include "cmd/automap/ddp.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <memory>
#include <string>

#include "absl/cleanup/cleanup.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"

absl::StatusOr<std::unique_ptr<DDPConn>> DDPConn::Create(
    const std::string& hostname, int port, const Options& options) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo* res;
  if (int rc = getaddrinfo(hostname.c_str(), nullptr, &hints, &res); rc != 0) {
    return absl::UnknownError(absl::StrFormat("getaddrinfo failed for %s: %s",
                                              hostname, gai_strerror(rc)));
  }
  absl::Cleanup res_deleter = [res] { freeaddrinfo(res); };

  LOG(INFO) << "hostname " << hostname << " is "
            << inet_ntoa(reinterpret_cast<struct sockaddr_in*>(res->ai_addr)
                             ->sin_addr);

  auto sockaddr = std::make_unique<struct sockaddr_in>();
  QCHECK_EQ(res->ai_addrlen, sizeof(struct sockaddr_in));
  memcpy(sockaddr.get(), res->ai_addr, sizeof(struct sockaddr_in));
  sockaddr->sin_port = htons(port);

  int s;
  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    return absl::ErrnoToStatus(s, "failed to create socket");
  }

  return std::unique_ptr<DDPConn>(new DDPConn(s, std::move(sockaddr), options));
}

absl::Status DDPConn::SetAll(int color) {
  std::vector<char> chans(options_.num_pixels * 3);
  for (int i = 0; i < options_.num_pixels * 3; i += 3) {
    chans[i] = color >> 16;
    chans[i + 1] = (color >> 8) & 0xff;
    chans[i + 2] = color & 0xff;
  }

  return SetAll(chans);
}

absl::Status DDPConn::OnlyOne(int idx, int color) {
  std::vector<char> chans(options_.num_pixels * 3);
  idx *= 3;
  chans[idx++] = color >> 16;
  chans[idx++] = (color >> 8) & 0xff;
  chans[idx] = color & 0xff;

  return SetAll(chans);
}

absl::Status DDPConn::SetAll(const std::vector<char>& chans) {
  absl::Span<const char> span(chans);
  int packet_num = 0, off = 0;
  while (!span.empty()) {
    ++packet_num;
    absl::Span<const char> packet_span =
        span.subspan(0, options_.max_chans_per_packet);
    span.remove_prefix(packet_span.size());
    const bool last = span.empty();

    std::vector<unsigned char> packet = MakePacket(packet_span, off, last);
    absl::Status status = SendPacket(packet);
    if (!status.ok()) {
      return status;
    }

    off += packet_span.size();
  }

  return absl::OkStatus();
}

namespace {

struct DdpHeader {
  unsigned char flags;
  unsigned char seq;
  unsigned char data_type;
  unsigned char id;
  uint32_t offset;
  uint16_t len;
  unsigned char data[];
} __attribute__((packed));

constexpr unsigned char DDP_FLAGS_VER1 = 0x40;
constexpr unsigned char DDP_FLAGS_PUSH = 0x01;

}  // namespace

std::vector<unsigned char> DDPConn::MakePacket(absl::Span<const char> chans,
                                               int off, bool last) {
  std::vector<unsigned char> packet(sizeof(struct DdpHeader) + chans.size());
  DdpHeader* hdr = reinterpret_cast<DdpHeader*>(packet.data());

  hdr->flags = DDP_FLAGS_VER1 | (last ? DDP_FLAGS_PUSH : 0);
  hdr->seq = GetSeq();
  hdr->data_type = 1;  // What xLights uses
  hdr->id = 1;
  hdr->offset = off;
  hdr->len = chans.size();
  memcpy(hdr->data, chans.data(), chans.size());

  return packet;
}

absl::Status DDPConn::SendPacket(absl::Span<const unsigned char> packet) {
  if (sendto(sock_, packet.data(), packet.size(), 0,
             reinterpret_cast<sockaddr*>(addr_.get()),
             sizeof(struct sockaddr_in)) < 0) {
    return absl::ErrnoToStatus(errno, "failed to send packet");
  }

  return absl::OkStatus();
}

int DDPConn::GetSeq() {
  int seq = seq_;
  ++seq_;
  if (seq_ > 15) {
    seq_ = 1;
  }
  return seq;
}
