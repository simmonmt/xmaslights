#include "cmd/automap/net.h"

#include <string>
#include <tuple>
#include <vector>

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"

std::tuple<std::string, int> ParseHostPort(const std::string& str,
                                           int default_port) {
  static const auto kInvalid = std::make_tuple("", 0);

  std::vector<std::string> v = absl::StrSplit(str, absl::MaxSplits(',', 1));
  switch (v.size()) {
    case 0:
      return kInvalid;
    case 1:
      return std::make_tuple(str, default_port);
    default: {
      int port;
      if (!absl::SimpleAtoi(v[1], &port) || port < 0 || port > 65535) {
        return kInvalid;
      }
      return std::make_tuple(v[0], port);
    }
  }
}
