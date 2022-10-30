#ifndef _LIB_FILE_WRITERS_H_
#define _LIB_FILE_WRITERS_H_ 1

#include <string>

#include "absl/status/status.h"
#include "absl/types/span.h"

absl::Status WriteFile(const std::string& path,
                       absl::Span<const std::string> lines);

#endif  // _LIB_FILE_WRITERS_H_
