#include "lib/geometry/points_testutil.h"

#include "lib/geometry/points.h"

namespace {

using ::testing::DoubleNear;
using ::testing::ExplainMatchResult;
using ::testing::MatchResultListener;

class XYZPosNearBaseMatcher {
 public:
  XYZPosNearBaseMatcher(double within) : within_(within) {}

  using is_gtest_matcher = void;

  void DescribeTo(std::ostream* os) const {
    *os << "is within or equal to " << within_;
  }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not within or equal to " << within_;
  }

 protected:
  bool MatchAndExplain(const XYZPos& want, const XYZPos& got,
                       MatchResultListener* os) const {
    return ExplainMatchResult(DoubleNear(want.x, within_), got.x, os) &&
           ExplainMatchResult(DoubleNear(want.y, within_), got.y, os) &&
           ExplainMatchResult(DoubleNear(want.z, within_), got.z, os);
  }

 private:
  const double within_;
};

class XYZPosNearTupleMatcher : public XYZPosNearBaseMatcher {
 public:
  XYZPosNearTupleMatcher(double within) : XYZPosNearBaseMatcher(within) {}

  bool MatchAndExplain(const std::tuple<XYZPos, XYZPos>& arg,
                       MatchResultListener* os) const {
    return XYZPosNearBaseMatcher::MatchAndExplain(std::get<0>(arg),
                                                  std::get<1>(arg), os);
  }
};

class XYZPosNearMatcher : public XYZPosNearBaseMatcher {
 public:
  XYZPosNearMatcher(const XYZPos& want, double within)
      : XYZPosNearBaseMatcher(within), want_(want) {}

  bool MatchAndExplain(const XYZPos& got, MatchResultListener* os) const {
    return XYZPosNearBaseMatcher::MatchAndExplain(want_, got, os);
  }

 private:
  const XYZPos want_;
};

}  // namespace

::testing::Matcher<std::tuple<XYZPos, XYZPos>> XYZPosNear(double within) {
  return XYZPosNearTupleMatcher(within);
}

::testing::Matcher<XYZPos> XYZPosNear(const XYZPos& want, double within) {
  return XYZPosNearMatcher(want, within);
}
