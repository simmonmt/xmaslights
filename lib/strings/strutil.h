#ifndef _LIB_STRINGS_STRUTIL_H_
#define _LIB_STRINGS_STRUTIL_H_ 1

#include <string>

#include "absl/types/span.h"

std::string IndexesToRanges(absl::Span<const int> indexes);

#endif  // _LIB_STRINGS_STRUTIL_H_
