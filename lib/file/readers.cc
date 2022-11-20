#include "lib/file/readers.h"

#include <fstream>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"

absl::StatusOr<std::vector<int>> ReadInts(const std::string& path) {
  std::vector<int> out;
  absl::Status status =
      ReadLines(path, [&](int lineno, const std::string& line) {
        int num;
        if (!absl::SimpleAtoi(line, &num)) {
          return MakeParseError(path, lineno, "bad number");
        }
        out.push_back(num);
        return absl::OkStatus();
      });

  if (!status.ok()) {
    return status;
  }
  return out;
}

absl::Status ReadLines(
    const std::string& path,
    std::function<absl::Status(int lineno, const std::string&)> callback) {
  std::ifstream infile(path);
  if (!infile.good()) {
    return absl::UnknownError("failed to open file");
  }

  std::vector<int> out;
  std::string line;
  for (int i = 1; std::getline(infile, line); ++i) {
    if (absl::Status status = callback(i, line); !status.ok()) {
      return status;
    }
  }

  if (!infile.good() && !infile.eof()) {
    return absl::UnknownError("read failure");
  }
  return absl::OkStatus();
}

absl::Status ReadFields(
    const std::string& path, const std::string& delim, int num_fields,
    std::function<absl::Status(int lineno, absl::Span<const std::string>)>
        callback) {
  return ReadLines(path, [&](int lineno, const std::string& line) {
    std::vector<std::string> parts = absl::StrSplit(line, delim);
    if (num_fields > 0 &&
        parts.size() != static_cast<unsigned long>(num_fields)) {
      return MakeParseError(path, lineno,
                            absl::StrFormat("wanted %d fields, got %d",
                                            num_fields, parts.size()));
    }

    return callback(lineno, parts);
  });
}

absl::Status MakeParseError(const std::string& path, int lineno,
                            const std::string& suffix) {
  return absl::InvalidArgumentError(
      absl::StrFormat("%s:%d: %s", path, lineno, suffix));
}
