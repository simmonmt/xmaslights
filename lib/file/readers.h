#ifndef _LIB_FILE_READERS_H_
#define _LIB_FILE_READERS_H_ 1

#include <string>
#include <vector>

#include "absl/status/statusor.h"

absl::StatusOr<std::vector<int>> ReadInts(const std::string& path);

absl::Status ReadLines(
    const std::string& path,
    std::function<absl::Status(int lineno, const std::string&)> callback);

absl::Status ReadFields(
    const std::string& path, const std::string& delim, int num_fields,
    std::function<absl::Status(int lineno, absl::Span<const std::string>)>
        callback);

absl::Status MakeParseError(const std::string& path, int lineno,
                            const std::string& suffix);

#endif  // _LIB_FILE_READERS_H_
