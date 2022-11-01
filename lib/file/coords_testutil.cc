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
  CoordsRecordEqBaseMatcher(double world_within)
      : world_within_(world_within) {}

  using is_gtest_matcher = void;

  void DescribeTo(std::ostream* os) const {
    *os << "is world_within or equal to " << world_within_;
  }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not world_within or equal to " << world_within_;
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

    if (want.world_coord.has_value() != got.world_coord.has_value()) {
      *os << "want " << (want.world_coord.has_value() ? "" : "no ")
          << " world coord; "
          << "got " << (got.world_coord.has_value() ? "" : "no ")
          << " world coord";
      matched = false;
    } else if (want.world_coord.has_value()) {
      matched &=
          ExplainMatchResult(DoubleNear(want.world_coord->x, world_within_),
                             got.world_coord->x, os);
      matched &=
          ExplainMatchResult(DoubleNear(want.world_coord->y, world_within_),
                             got.world_coord->y, os);
      matched &=
          ExplainMatchResult(DoubleNear(want.world_coord->z, world_within_),
                             got.world_coord->z, os);
    }

    return matched;
  }

 private:
  const double world_within_;
};

class CoordsRecordEqTupleMatcher : public CoordsRecordEqBaseMatcher {
 public:
  CoordsRecordEqTupleMatcher(double world_within)
      : CoordsRecordEqBaseMatcher(world_within) {}

  bool MatchAndExplain(const std::tuple<CoordsRecord, CoordsRecord>& arg,
                       MatchResultListener* os) const {
    return CoordsRecordEqBaseMatcher::MatchAndExplain(std::get<0>(arg),
                                                      std::get<1>(arg), os);
  }
};

class CoordsRecordEqMatcher : public CoordsRecordEqBaseMatcher {
 public:
  CoordsRecordEqMatcher(const CoordsRecord& want, double world_within)
      : CoordsRecordEqBaseMatcher(world_within), want_(want) {}

  bool MatchAndExplain(const CoordsRecord& got, MatchResultListener* os) const {
    return CoordsRecordEqBaseMatcher::MatchAndExplain(want_, got, os);
  }

 private:
  const CoordsRecord want_;
};

}  // namespace

::testing::Matcher<std::tuple<CoordsRecord, CoordsRecord>> CoordsRecordEq(
    double world_within) {
  return CoordsRecordEqTupleMatcher(world_within);
}

::testing::Matcher<CoordsRecord> CoordsRecordEq(const CoordsRecord& want,
                                                double world_within) {
  return CoordsRecordEqMatcher(want, world_within);
}
