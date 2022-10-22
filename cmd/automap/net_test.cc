#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "absl/strings/str_replace.h"
#include "cmd/automap/net.h"

namespace {

using ::testing::FieldsAre;
using ::testing::ValuesIn;

struct TestCase {
  std::string str;
  std::string want_host;
  int want_port;
};

constexpr int kDefaultPort = 9999;

const TestCase kTestCases[] = {
    {"foo:1234", "foo", 1234},
    {"foo", "foo", kDefaultPort},
    {"foo:abcd", "", 0},
};

class ParseHostPortTest : public testing::TestWithParam<TestCase> {};

TEST_P(ParseHostPortTest, Test) {
  const TestCase& test_case = GetParam();

  EXPECT_THAT(ParseHostPort(test_case.str, kDefaultPort),
              FieldsAre(test_case.want_host, test_case.want_port));
}

INSTANTIATE_TEST_SUITE_P(
    TestCases, ParseHostPortTest, ValuesIn(kTestCases),
    [](const ::testing::TestParamInfo<TestCase>& info) {
      return absl::StrReplaceAll(info.param.str, {{":", "_"}});
    });

}  // namespace
