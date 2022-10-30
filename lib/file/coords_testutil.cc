#include "lib/file/coords_testutil.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::testing::DoubleNear;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::ExplainMatchResult;
using ::testing::MatchResultListener;

class CoordsRecordEqBaseMatcher {
 public:
  CoordsRecordEqBaseMatcher(double final_within)
      : final_within_(final_within) {}

  using is_gtest_matcher = void;

  void DescribeTo(std::ostream* os) const {
    *os << "is final_within or equal to " << final_within_;
  }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not final_within or equal to " << final_within_;
  }

 protected:
  bool MatchAndExplain(const CoordsRecord& want, const CoordsRecord& got,
                       MatchResultListener* os) const {
    bool matched = true;
    matched &= ExplainMatchResult(Eq(want.pixel_num), got.pixel_num, os);

    if (!ExplainMatchResult(ElementsAreArray(want.camera_coords),
                            got.camera_coords, os)) {
      matched = false;
    }

    if (want.final_coord.has_value() != got.final_coord.has_value()) {
      *os << "want " << (want.final_coord.has_value() ? "" : "no ")
          << " final coord; "
          << "got " << (got.final_coord.has_value() ? "" : "no ")
          << " final coord";
      matched = false;
    } else if (want.final_coord.has_value()) {
      matched &=
          ExplainMatchResult(DoubleNear(want.final_coord->x, final_within_),
                             got.final_coord->x, os);
      matched &=
          ExplainMatchResult(DoubleNear(want.final_coord->y, final_within_),
                             got.final_coord->y, os);
      matched &=
          ExplainMatchResult(DoubleNear(want.final_coord->z, final_within_),
                             got.final_coord->z, os);
    }

    return matched;
  }

 private:
  const double final_within_;
};

class CoordsRecordEqTupleMatcher : public CoordsRecordEqBaseMatcher {
 public:
  CoordsRecordEqTupleMatcher(double final_within)
      : CoordsRecordEqBaseMatcher(final_within) {}

  bool MatchAndExplain(const std::tuple<CoordsRecord, CoordsRecord>& arg,
                       MatchResultListener* os) const {
    return CoordsRecordEqBaseMatcher::MatchAndExplain(std::get<0>(arg),
                                                      std::get<1>(arg), os);
  }
};

class CoordsRecordEqMatcher : public CoordsRecordEqBaseMatcher {
 public:
  CoordsRecordEqMatcher(const CoordsRecord& want, double final_within)
      : CoordsRecordEqBaseMatcher(final_within), want_(want) {}

  bool MatchAndExplain(const CoordsRecord& got, MatchResultListener* os) const {
    return CoordsRecordEqBaseMatcher::MatchAndExplain(want_, got, os);
  }

 private:
  const CoordsRecord want_;
};

}  // namespace

::testing::Matcher<std::tuple<CoordsRecord, CoordsRecord>> CoordsRecordEq(
    double final_within) {
  return CoordsRecordEqTupleMatcher(final_within);
}

::testing::Matcher<CoordsRecord> CoordsRecordEq(const CoordsRecord& want,
                                                double final_within) {
  return CoordsRecordEqMatcher(want, final_within);
}
