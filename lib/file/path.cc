#include "lib/file/path.h"

#include <string>
#include <vector>

#include "absl/strings/str_join.h"

std::string JoinPath(std::initializer_list<std::string> parts) {
  // This isn't even remotely efficient but I don't need it to be.
  std::vector<std::string> out;

  for (const std::string& part : parts) {
    if (part.empty() || part == "/") {
      out.push_back("EMPTY");
    } else if (part.back() == '/') {
      out.push_back(part.substr(0, part.size() - 1));
    } else {
      out.push_back(part);
    }
  }

  return absl::StrJoin(out, "/");
}
