#include "cmd/showfound/view_command.h"

#include <optional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "opencv2/core/types.hpp"

namespace {

using ::testing::Eq;
using ::testing::HasSubstr;

TEST(CommandBufferTest, FullSequence) {
  CommandBuffer buf;

  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());

  buf.AddKey('1');
  EXPECT_THAT(buf.prefix(), Eq(1));
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());

  buf.AddKey('3');
  EXPECT_THAT(buf.prefix(), Eq(13));
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());

  buf.AddKey('x');
  EXPECT_THAT(buf.prefix(), Eq(13));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_FALSE(buf.clicked());

  buf.AddClick();
  EXPECT_THAT(buf.prefix(), Eq(13));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_TRUE(buf.clicked());

  buf.Reset();
  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());
}

TEST(CommandBufferTest, NoPrefix) {
  CommandBuffer buf;

  buf.AddKey('x');
  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_FALSE(buf.clicked());

  buf.AddClick();
  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_TRUE(buf.clicked());
}

TEST(CommandBufferTest, KeyRestarts) {
  CommandBuffer buf;

  buf.AddKey('1');
  buf.AddKey('x');
  EXPECT_THAT(buf.prefix(), Eq(1));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_FALSE(buf.clicked());

  buf.AddKey('y');
  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), Eq('y'));
  EXPECT_FALSE(buf.clicked());

  buf.AddClick();
  buf.AddKey('z');
  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), Eq('z'));
  EXPECT_FALSE(buf.clicked());
}

TEST(CommandBufferTest, NumberRestarts) {
  CommandBuffer buf;

  buf.AddKey('3');
  buf.AddKey('x');
  EXPECT_THAT(buf.prefix(), Eq(3));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_FALSE(buf.clicked());

  buf.AddKey('1');
  EXPECT_THAT(buf.prefix(), Eq(1));
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());

  buf.AddKey('x');
  EXPECT_THAT(buf.prefix(), Eq(1));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_FALSE(buf.clicked());

  buf.AddClick();
  EXPECT_THAT(buf.prefix(), Eq(1));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_TRUE(buf.clicked());

  buf.AddKey('2');
  EXPECT_THAT(buf.prefix(), Eq(2));
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());
}

TEST(CommandBufferTest, SecondClick) {
  CommandBuffer buf;
  buf.AddKey('3');
  buf.AddKey('x');
  buf.AddClick();
  buf.AddClick();

  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_TRUE(buf.clicked());
}

TEST(KeymapTest, AddKey) {
  bool ran_command = false;
  Keymap keymap;
  keymap.AddKey('c', "no arg", [&] { ran_command = true; });

  cv::Point2i mouse(10, 20);

  CommandBuffer buf;
  buf.AddKey('x');
  EXPECT_THAT(keymap.Execute(buf, std::nullopt, mouse), Eq(Keymap::UNKNOWN));
  EXPECT_FALSE(ran_command);

  buf.Reset();
  buf.AddKey('c');
  ASSERT_THAT(keymap.Execute(buf, std::nullopt, mouse), Eq(Keymap::EXECUTED));
  ASSERT_TRUE(ran_command);

  ran_command = false;
  buf.Reset();
  buf.AddKey('1');
  buf.AddKey('c');
  EXPECT_THAT(keymap.Execute(buf, std::nullopt, mouse), Eq(Keymap::ERROR));
  EXPECT_FALSE(ran_command);
}

TEST(KeymapTest, AddReqPrefixKey) {
  std::optional<int> ran_command;

  Keymap keymap;
  keymap.AddReqPrefixKey('c', "prefix arg",
                         [&](int prefix) { ran_command = prefix; });

  cv::Point2i mouse(10, 20);

  CommandBuffer buf;
  buf.AddKey('x');
  EXPECT_THAT(keymap.Execute(buf, std::nullopt, mouse), Eq(Keymap::UNKNOWN));

  buf.Reset();
  buf.AddKey('c');
  ASSERT_THAT(keymap.Execute(buf, std::nullopt, mouse), Eq(Keymap::ERROR));
  ASSERT_THAT(ran_command, std::nullopt);

  buf.Reset();
  buf.AddKey('1');
  buf.AddKey('c');
  ASSERT_THAT(keymap.Execute(buf, std::nullopt, mouse), Eq(Keymap::EXECUTED));
  ASSERT_EQ(ran_command, 1);
}

TEST(KeymapDeathTest, DoubleAdd) {
  EXPECT_DEATH(
      {
        Keymap keymap;
        keymap.AddKey('n', "...", [&] {});
        keymap.AddKey('n', "...", [&] {});
      },
      HasSubstr("duplicate"));

  EXPECT_DEATH(
      {
        Keymap keymap;
        keymap.AddKey('n', "...", [&] {});
        keymap.AddReqPrefixKey('n', "...", [&](int prefix) {});
      },
      HasSubstr("duplicate"));
}

}  // namespace
