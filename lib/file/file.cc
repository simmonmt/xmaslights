#include "lib/file/file.h"

#include <sys/errno.h>
#include <sys/stat.h>

#include "absl/strings/str_format.h"

absl::StatusOr<bool> Exists(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) < 0) {
    if (errno == ENOENT) {
      return false;
    }

    return absl::ErrnoToStatus(
        errno, absl::StrFormat("checking for existence of %s", path));
  }

  return true;
}
