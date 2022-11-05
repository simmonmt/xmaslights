#include "lib/strings/strutil.h"

#include <optional>
#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/types/span.h"

std::string IndexesToRanges(absl::Span<const int> indexes) {
  std::vector<std::pair<int, int>> ranges;
  std::optional<std::pair<int, int>> cur_range;

  for (const int idx : indexes) {
    if (!cur_range.has_value()) {
      cur_range = std::make_pair(idx, idx);
      continue;
    }

    if (cur_range->second + 1 == idx) {
      cur_range->second = idx;
      continue;
    }

    ranges.push_back(*cur_range);
    cur_range = std::make_pair(idx, idx);
  }

  if (cur_range.has_value()) {
    ranges.push_back(*cur_range);
  }

  std::vector<std::string> out;
  for (const auto& [from, to] : ranges) {
    if (from == to) {
      out.push_back(absl::StrCat(from));
    } else {
      out.push_back(absl::StrCat(from, "-", to));
    }
  }

  return absl::StrJoin(out, ",");
}
