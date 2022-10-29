#include "lib/geometry/points.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"

std::ostream& operator<<(std::ostream& os, const XYPos& pos) {
  return os << pos.x << "," << pos.y;
}

std::ostream& operator<<(std::ostream& os, const XYZPos& pos) {
  return os << pos.x << "," << pos.y << "," << pos.z;
}

absl::FormatConvertResult<absl::FormatConversionCharSet::kString |
                          absl::FormatConversionCharSet::kIntegral>
AbslFormatConvert(const XYPos& pos, const absl::FormatConversionSpec& spec,
                  absl::FormatSink* s) {
  if (spec.conversion_char() == absl::FormatConversionChar::s) {
    s->Append(absl::StrCat("x=", pos.x, ",y=", pos.y));
  } else {
    s->Append(absl::StrCat("", pos.x, ",", pos.y));
  }
  return {true};
}

absl::FormatConvertResult<absl::FormatConversionCharSet::kString |
                          absl::FormatConversionCharSet::kIntegral>
AbslFormatConvert(const XYZPos& pos, const absl::FormatConversionSpec& spec,
                  absl::FormatSink* s) {
  if (spec.conversion_char() == absl::FormatConversionChar::s) {
    s->Append(absl::StrCat("x=", pos.x, ",y=", pos.y, ",z=", pos.z));
  } else {
    s->Append(absl::StrCat("", pos.x, ",", pos.y, ",", pos.z));
  }
  return {true};
}

void PointsIndex::Add(const XYZPos& pos, double key) {
  index_.emplace(key, pos);
}

std::vector<XYZPos> PointsIndex::Near(double where, double within) {
  std::vector<XYZPos> out;
  double max_upper = where + within;
  for (auto iter = index_.lower_bound(where - within); iter != index_.end();
       ++iter) {
    if (iter->first > max_upper) {
      break;
    }
    out.push_back(iter->second);
  }
  return out;
}
