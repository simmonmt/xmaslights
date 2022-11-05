#include "cmd/showfound/view_command.h"

#include <optional>

#include "absl/log/log.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "opencv2/core/types.hpp"

namespace {

using ::testing::_;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::VariantWith;

TEST(CommandBufferTest, FullSequence) {
  CommandBuffer buf;

  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);

  buf.AddKey('1');
  EXPECT_THAT(buf.prefix(), Eq(1));
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);

  buf.AddKey('3');
  EXPECT_THAT(buf.prefix(), Eq(13));
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);

  buf.AddKey('x');
  EXPECT_THAT(buf.prefix(), Eq(13));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_FALSE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);

  buf.AddClick();
  EXPECT_THAT(buf.prefix(), Eq(13));
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_TRUE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);

  buf.Reset();
  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);
}

TEST(CommandBufferTest, Errors) {
  CommandBuffer buf;

  buf.AddKey('1');
  buf.AddKey('x');
  buf.AddClick();
  buf.SetError(CommandBuffer::UNKNOWN);

  EXPECT_THAT(buf.prefix(), std::nullopt);
  EXPECT_THAT(buf.key(), std::nullopt);
  EXPECT_FALSE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::UNKNOWN);

  buf.AddKey('1');
  EXPECT_THAT(buf.prefix(), Eq(1));
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);

  buf.SetError(CommandBuffer::UNKNOWN);
  buf.AddKey('x');
  EXPECT_THAT(buf.key(), Eq('x'));
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);

  buf.SetError(CommandBuffer::UNKNOWN);
  buf.AddClick();
  EXPECT_TRUE(buf.clicked());
  EXPECT_THAT(buf.error(), CommandBuffer::NONE);
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

TEST(BareCommandTest, ReturnCodes) {
  for (Command::ExecuteResult result :
       {Command::USAGE, Command::ERROR, Command::EXEC_OK}) {
    auto command =
        std::make_unique<BareCommand>('c', "help", [&] { return result; });
    EXPECT_THAT(command->Execute({}), Eq(result)) << result;
  }
}

TEST(BareCommandTest, Command) {
  auto command = std::make_unique<BareCommand>(
      'c', "help", [&] { return Command::EXEC_OK; });
  EXPECT_THAT(command->key(), Eq('c'));
  EXPECT_THAT(command->Execute({}), Eq(Command::EXEC_OK));
  EXPECT_THAT(command->Execute({.prefix = 4}), Eq(Command::USAGE));
}

TEST(PrefixCommandTest, ReturnCodes) {
  for (Command::ExecuteResult result :
       {Command::USAGE, Command::ERROR, Command::EXEC_OK}) {
    auto command = std::make_unique<PrefixCommand>('c', "help",
                                                   [&](int) { return result; });
    EXPECT_THAT(command->Execute({.prefix = 1}), Eq(result)) << result;
  }
}

TEST(PrefixCommandTest, Command) {
  int saved_num = -1;
  auto command = std::make_unique<PrefixCommand>('c', "help", [&](int num) {
    saved_num = num;
    return Command::EXEC_OK;
  });

  EXPECT_THAT(command->key(), Eq('c'));

  saved_num = -1;
  EXPECT_THAT(command->Execute({.prefix = 4}), Eq(Command::EXEC_OK));
  EXPECT_EQ(saved_num, 4);

  saved_num = -1;
  EXPECT_THAT(command->Execute({.prefix = 4, .focus = 5, .over = 6}),
              Eq(Command::EXEC_OK));
  EXPECT_EQ(saved_num, 4);

  saved_num = -1;
  EXPECT_THAT(command->Execute({}), Eq(Command::USAGE));
  EXPECT_EQ(saved_num, -1);
}

TEST(OverUnlessPrefixCommandTest, ReturnCodes) {
  for (Command::ExecuteResult result :
       {Command::USAGE, Command::ERROR, Command::EXEC_OK}) {
    auto command = std::make_unique<OverUnlessPrefixCommand>(
        'c', "help", [&](int) { return result; });
    EXPECT_THAT(command->Execute({.prefix = 1}), Eq(result)) << result;
  }
}

TEST(OverUnlessPrefixCommandTest, Command) {
  int saved_num = -1;
  auto command =
      std::make_unique<OverUnlessPrefixCommand>('c', "help", [&](int num) {
        saved_num = num;
        return Command::EXEC_OK;
      });

  EXPECT_THAT(command->key(), Eq('c'));

  saved_num = -1;
  EXPECT_THAT(command->Execute({.prefix = 4}), Eq(Command::EXEC_OK));
  EXPECT_EQ(saved_num, 4);

  saved_num = -1;
  EXPECT_THAT(command->Execute({.over = 5}), Eq(Command::EXEC_OK));
  EXPECT_EQ(saved_num, 5);

  saved_num = -1;
  EXPECT_THAT(command->Execute({.prefix = 4, .focus = 5, .over = 6}),
              Eq(Command::EXEC_OK));
  EXPECT_EQ(saved_num, 4);

  saved_num = -1;
  EXPECT_THAT(command->Execute({}), Eq(Command::USAGE));
  EXPECT_EQ(saved_num, -1);
}

TEST(KeymapTest, Lookup) {
  Keymap keymap;
  keymap.Add(std::make_unique<BareCommand>('a', "command a",
                                           [&] { return Command::EXEC_OK; }));
  keymap.Add(std::make_unique<BareCommand>('b', "command b",
                                           [&] { return Command::EXEC_OK; }));

  CommandBuffer buf;

  buf.AddKey('1');
  EXPECT_THAT(keymap.Lookup(buf),
              VariantWith<Keymap::LookupResult>(Keymap::CONTINUE));

  buf.AddKey('a');
  EXPECT_THAT(keymap.Lookup(buf), VariantWith<const Command*>(Property(
                                      &Command::help, Eq("command a"))));

  buf.AddKey('b');
  EXPECT_THAT(keymap.Lookup(buf), VariantWith<const Command*>(Property(
                                      &Command::help, Eq("command b"))));

  buf.AddKey('c');
  EXPECT_THAT(keymap.Lookup(buf),
              VariantWith<Keymap::LookupResult>(Keymap::UNKNOWN));

  buf.AddKey('?');  // builtin command
  EXPECT_THAT(keymap.Lookup(buf), VariantWith<const Command*>(_));
}

// TEST(KeymapTest, AddKey) {
//   bool ran_command = false;
//   Keymap keymap;
//   keymap.AddKey('c', "no arg", [&] { ran_command = true; });

//   Keymap::ExecuteArgs args = {
//     .focus = std::nullopt,
//     .over = std::nullopt,
//     .mouse_coords = cv::Point2i(10,20),
//   };

//   CommandBuffer buf;
//   buf.AddKey('x');
//   EXPECT_THAT(keymap.Execute(buf, args), Eq(Keymap::UNKNOWN));
//   EXPECT_FALSE(ran_command);

//   buf.Reset();
//   buf.AddKey('c');
//   ASSERT_THAT(keymap.Execute(buf, args), Eq(Keymap::EXECUTED));
//   ASSERT_TRUE(ran_command);

//   ran_command = false;
//   buf.Reset();
//   buf.AddKey('1');
//   buf.AddKey('c');
//   EXPECT_THAT(keymap.Execute(buf, args), Eq(Keymap::ERROR));
//   EXPECT_FALSE(ran_command);
// }

// TEST(KeymapTest, AddReqPrefixKey) {
//   std::optional<int> ran_command;

//   Keymap keymap;
//   keymap.AddReqPrefixKey('c', "prefix arg",
//                          [&](int prefix) { ran_command = prefix; });

//   Keymap::ExecuteArgs args = {
//     .focus = std::nullopt,
//     .over = std::nullopt,
//     .mouse_coords = cv::Point2i(10,20),
//   };

//   CommandBuffer buf;
//   buf.AddKey('x');
//   EXPECT_THAT(keymap.Execute(buf, args), Eq(Keymap::UNKNOWN));

//   buf.Reset();
//   buf.AddKey('c');
//   ASSERT_THAT(keymap.Execute(buf, args), Eq(Keymap::ERROR));
//   ASSERT_THAT(ran_command, std::nullopt);

//   buf.Reset();
//   buf.AddKey('1');
//   buf.AddKey('c');
//   ASSERT_THAT(keymap.Execute(buf, args), Eq(Keymap::EXECUTED));
//   ASSERT_EQ(ran_command, 1);
// }

// TEST(KeymapDeathTest, DoubleAdd) {
//   EXPECT_DEATH(
//       {
//         Keymap keymap;
//         keymap.AddKey('n', "...", [&] {});
//         keymap.AddKey('n', "...", [&] {});
//       },
//       HasSubstr("duplicate"));

//   EXPECT_DEATH(
//       {
//         Keymap keymap;
//         keymap.AddKey('n', "...", [&] {});
//         keymap.AddReqPrefixKey('n', "...", [&](int prefix) {});
//       },
//       HasSubstr("duplicate"));
// }

}  // namespace
