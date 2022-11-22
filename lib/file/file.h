#ifndef _LIB_FILE_FILE_H_
#define _LIB_FILE_FILE_H_ 1

#include <string>

#include "absl/status/statusor.h"

absl::StatusOr<bool> Exists(const std::string& path);

#endif  // _LIB_FILE_FILE_H_
