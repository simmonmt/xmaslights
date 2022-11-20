#include "lib/file/xlights.h"

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/file/path.h"
#include "lib/file/readers.h"

namespace {

using ::testing::AllOf;
using ::testing::ContainerEq;
using ::testing::Field;
using ::testing::Pointee;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

std::string Path(const std::string& relpath) {
  return JoinPath({::testing::TempDir(), relpath});
}

TEST(XLightsModelCreatorTest, NoCollisions) {
  // A central point at 10,10,10 with axis-aligned points on each
  // axis. There shouldn't be any collisions because all points are at
  // least 1 apart. The central point is surrounded on both sides on
  // each axis just to be sure we're checking collisions in all
  // directions. Each axis has a different number of points to ensure
  // the x/y/zsize is calculated correctly.
  const std::vector<const std::pair<int, cv::Point3d>> input = {
      {0, {10, 11, 11}},                     // below center on X
      {1, {11, 10, 11}},                     // below center on Y
      {2, {11, 11, 10}},                     // below center on Z
      {3, {11, 11, 11}},                     // center
      {4, {12, 11, 11}},                     // above center on X
      {5, {11, 12, 11}}, {6, {11, 13, 11}},  // above center on Y
      {7, {11, 11, 12}}, {8, {11, 11, 13}}, {9, {11, 11, 14}},  // above on Z
  };

  // The output is translated such that all coordinate
  // values are >=0.
  const std::map<std::tuple<int, int, int>, int> want = {
      {{0, 3, 1}, 0},                  // below center on X
      {{1, 3, 0}, 1},                  // below center on Z (was -Y)
      {{1, 4, 1}, 2},                  // above center on Y (was +Z)
      {{1, 3, 1}, 3},                  // center
      {{2, 3, 1}, 4},                  // above center on X
      {{1, 3, 2}, 5}, {{1, 3, 3}, 6},  // above center on Z (were +Y)
      {{1, 2, 1}, 7}, {{1, 1, 1}, 8}, {{1, 0, 1}, 9},  // below on Y (was -Z)
  };

  XLightsModelCreator creator("model");
  creator.set_initial_scaling_factor(1.0);

  EXPECT_THAT(
      creator.CreateModel(input),
      Pointee(AllOf(Field("name", &XLightsModel::name, "model"),
                    Field("points", &XLightsModel::points, ContainerEq(want)),
                    Field("collisions", &XLightsModel::collisions,
                          UnorderedElementsAre()),
                    Field("scaling_factor", &XLightsModel::scaling_factor, 1.0),
                    Field("xsize", &XLightsModel::xsize, 3),
                    Field("ysize", &XLightsModel::ysize, 5),
                    Field("zsize", &XLightsModel::zsize, 4))));
}

TEST(XLightsModelCreatorTest, Collisions) {
  const std::vector<const std::pair<int, cv::Point3d>> input = {
      {0, {0, 0, 0}},  // collision on Y axis
      {1, {1, 0, 0}},

      {2, {10, 0, 0}},  // collision on Z axis (mapped from Y)
      {3, {10, 1, 0}},

      {4, {20, 0, 1}},  // collision on Y axis (mapped from -Z)
      {5, {20, 0, 0}},

      {6, {30, 1, 1}},    // test multiple collisions; this is the target
      {7, {31, 1, 1}},    // collides with 6 on X
      {8, {32, 1, 1}},    // survives; too far away on X
      {9, {31, 0, 1}},    // collides with 6 on Z
      {10, {31, 2, 1}},   // survives; too far away on Z
      {11, {31, 1, 0}},   // collides with 6 on -Y
      {12, {31, 1, -2}},  // survives; too far away on Y
  };

  const std::map<std::tuple<int, int, int>, int> want = {
      {{0, 0, 0}, 0},   // points 0 and 1
      {{5, 0, 0}, 2},   // points 2 and 3 (lowest wins)
      {{10, 0, 0}, 4},  // points 4 and 5 (lowest wins)
      {{15, 0, 0}, 6},  // points 6, 7, 9, and 11
      {{16, 0, 0}, 8}, {{15, 0, 1}, 10}, {{15, 1, 0}, 12},  // survivors
  };

  XLightsModelCreator creator("model");
  creator.set_initial_scaling_factor(2.0);
  creator.set_stop_size(0);  // stop after first iteration

  EXPECT_THAT(
      creator.CreateModel(input),
      Pointee(AllOf(Field("points", &XLightsModel::points, ContainerEq(want)),
                    Field("collisions", &XLightsModel::collisions,
                          UnorderedElementsAre(1, 3, 5, 7, 9, 11)),
                    Field("scaling_factor", &XLightsModel::scaling_factor, 2.0),
                    Field("xsize", &XLightsModel::xsize, 17),
                    Field("ysize", &XLightsModel::ysize, 2),
                    Field("zsize", &XLightsModel::zsize, 2))));
}

TEST(XLightsModelCreatorTest, Search) {
  const std::vector<const std::pair<int, cv::Point3d>> input = {
      {0, {0, 0, 0}}, {1, {0, 1, 0}}, {2, {1, 0, 0}},
      {3, {1, 1, 0}}, {4, {0, 0, 1}}, {5, {1, 1, 1}},

      {6, {2, 0, 0}}, {7, {0, 2, 0}}, {8, {0, 0, 2}},
  };

  XLightsModelCreator creator("model");
  creator.set_initial_scaling_factor(8.0);
  creator.set_stop_size(1'000'000);  // we shouldn't reach this

  EXPECT_THAT(creator.CreateModel(input),
              Pointee(AllOf(
                  Field("collisions", &XLightsModel::collisions, SizeIs(0)),
                  Field("scaling_factor", &XLightsModel::scaling_factor, 1))));
}

TEST(XLightsModelCreatorTest, StopSize) {
  // 10x10x10 cube that can't be turned into a model with scaling_factor >1
  // without collisions.
  std::vector<std::pair<int, cv::Point3d>> input;
  int num = 0;
  for (double x = 0; x < 10; ++x) {
    for (double y = 0; y < 10; ++y) {
      for (double z = 0; z < 10; ++z) {
        input.push_back(std::make_pair(num++, cv::Point3d{x, y, z}));
      }
    }
  }

  XLightsModelCreator creator("model");
  creator.set_initial_scaling_factor(10);

  struct TestCase {
    int stop_size;
    int want_collisions;
    double want_scaling_factor;
  };

  const std::vector<TestCase> test_cases = {
      {.stop_size = 0, .want_collisions = 999, .want_scaling_factor = 10},
      {.stop_size = 1, .want_collisions = 999, .want_scaling_factor = 10},
      {.stop_size = 2, .want_collisions = 992, .want_scaling_factor = 5},
      {.stop_size = 8, .want_collisions = 992, .want_scaling_factor = 5},
      {.stop_size = 9, .want_collisions = 936, .want_scaling_factor = 2.5},
      {.stop_size = 1000, .want_collisions = 0, .want_scaling_factor = 0.625},
  };

  for (const TestCase& test_case : test_cases) {
    creator.set_stop_size(test_case.stop_size);

    EXPECT_THAT(
        creator.CreateModel(input),
        Pointee(AllOf(Field("collisions", &XLightsModel::collisions,
                            SizeIs(test_case.want_collisions)),
                      Field("scaling_factor", &XLightsModel::scaling_factor,
                            test_case.want_scaling_factor))))
        << "stop size " << test_case.stop_size;
  }
}

TEST(WriteXLightsModelTest, Validate) {
  const std::vector<const std::pair<int, cv::Point3d>> input = {
      {0, {0, 1, 1}},                  //
      {1, {1, 0, 1}},                  //
      {2, {1, 1, 0}},                  //
      {3, {1, 1, 1}},                  //
      {4, {2, 1, 1}},                  //
      {5, {1, 2, 1}}, {6, {1, 3, 1}},  //
      {7, {1, 1, 2}}, {8, {1, 1, 4}},  //
  };

  XLightsModelCreator creator("model");
  creator.set_initial_scaling_factor(1.0);
  std::unique_ptr<XLightsModel> model = creator.CreateModel(input);

  const std::string path = Path("output");
  QCHECK_OK(WriteXLightsModel(*model, path));

  const std::vector<std::string> want = {
      R"(<?xml version="1.0" encoding="UTF-8"?>)",
      R"(<custommodel name="model")",
      R"(	     parm1="3")",  // width (xsize)
      R"(	     parm2="5")",  // height (ysize)
      R"(	     Depth="4")",  // depth (zsize)
      R"(	     StringType="RGB Nodes")",
      R"(	     Transparency="0")",
      R"(	     PixelSize="2")",
      R"(	     ModelBrightness="")",
      R"(	     Antialias="1")",
      R"(	     StrandNames="")",
      R"(	     NodeNames="")",
      R"(	     CustomModel=",,;,,;,,;,2,;,,|,9,;,,;,8,;1,4,5;,3,|,,;,,;,,;,6,;,,|,,;,,;,,;,7,;,,")",
      R"(	     SourceVersion="2020.37"  >)",
      R"(</custommodel>)",
  };

  std::vector<std::string> got;
  QCHECK_OK(ReadLines(path, [&](int lineno, const std::string& line) {
    got.push_back(line);
    return absl::OkStatus();
  }));

  for (const std::string& line : want) {
    LOG(INFO) << "want: " << line;
  }
  for (const std::string& line : got) {
    LOG(INFO) << "got: " << line;
  }

  ASSERT_EQ(want.size(), got.size());
  for (int i = 0; i < want.size(); ++i) {
    EXPECT_EQ(absl::StripAsciiWhitespace(want[i]),
              absl::StripAsciiWhitespace(got[i]))
        << "line " << i + 1;
  }
}

}  // namespace
