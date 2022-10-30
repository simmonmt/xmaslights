#include "lib/file/writers.h"

#include <fstream>
#include <string>

#include "absl/status/status.h"
#include "absl/types/span.h"

absl::Status WriteFile(const std::string& path,
                       absl::Span<const std::string> lines) {
  std::ofstream out;
  out.exceptions(0);
  out.open(path);
  if (!out.good()) {
    return absl::UnknownError("failed to open file for writing");
  }

  for (const std::string& line : lines) {
    out << line << "\n";
  }

  out.close();
  if (!out.good()) {
    return absl::UnknownError("failed to close file");
  }

  return absl::OkStatus();
}
