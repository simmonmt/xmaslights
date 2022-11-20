#include "cmd/showfound/view_command.h"

#include <optional>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/testing/cv.h"
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

TEST(ArgCommandTest, ReturnCodes) {
  for (Command::ExecuteResult result :
       {Command::USAGE, Command::ERROR, Command::EXEC_OK}) {
    auto command = std::make_unique<ArgCommand>('c', "help", ArgCommand::PREFIX,
                                                ArgCommand::PREFER,
                                                [&](int) { return result; });
    EXPECT_THAT(command->Execute({.prefix = 1}), Eq(result)) << result;
  }
}

TEST(ArgCommandTest, SingleSources) {
  std::vector<std::tuple<ArgCommand::ArgSource, Command::Args>> test_cases = {
      {ArgCommand::PREFIX, {.prefix = 1}},
      {ArgCommand::FOCUS, {.focus = 1}},
      {ArgCommand::OVER, {.over = 1}},
  };

  for (ArgCommand::ArgMode mode : {ArgCommand::PREFER, ArgCommand::EXCLUSIVE}) {
    for (auto& [source, args] : test_cases) {
      const std::string description =
          absl::StrFormat("source %d mode %d", source, mode);

      int save = 0;
      auto command =
          std::make_unique<ArgCommand>('c', "help", source, mode, [&](int arg) {
            save = arg;
            return Command::EXEC_OK;
          });

      EXPECT_EQ(command->Execute({}), Command::USAGE) << description;
      EXPECT_EQ(command->Execute(args), Command::EXEC_OK) << description;
      EXPECT_EQ(save, 1) << "source is " << description;
    }
  }
}

TEST(ArgCommandTest, Preference) {
  std::vector<std::tuple<unsigned int, Command::Args>> test_cases = {
      {ArgCommand::PREFIX | ArgCommand::FOCUS, {.prefix = 1, .focus = 2}},
      {ArgCommand::FOCUS | ArgCommand::OVER, {.focus = 1, .over = 2}},
      {ArgCommand::PREFIX | ArgCommand::OVER, {.prefix = 1, .over = 2}},
  };

  for (auto& [source, args] : test_cases) {
    const std::string description = absl::StrFormat("source %d", source);

    int save = 0;
    auto command = std::make_unique<ArgCommand>(
        'c', "help", source, ArgCommand::PREFER, [&](int arg) {
          save = arg;
          return Command::EXEC_OK;
        });

    EXPECT_EQ(command->Execute({}), Command::USAGE) << description;
    EXPECT_EQ(command->Execute(args), Command::EXEC_OK) << description;
    EXPECT_EQ(save, 1) << description;
  }
}

TEST(ArgCommandTest, Exclusive) {
  std::vector<std::tuple<bool, unsigned int, Command::Args>> test_cases = {
      {false,
       ArgCommand::PREFIX | ArgCommand::FOCUS,
       {.prefix = 1, .focus = 1}},
      {true, ArgCommand::PREFIX | ArgCommand::FOCUS, {.prefix = 1}},
      {true, ArgCommand::PREFIX | ArgCommand::FOCUS, {.focus = 1}},

      {false, ArgCommand::FOCUS | ArgCommand::OVER, {.focus = 1, .over = 1}},
      {true, ArgCommand::FOCUS | ArgCommand::OVER, {.focus = 1}},
      {true, ArgCommand::FOCUS | ArgCommand::OVER, {.over = 1}},

      {false, ArgCommand::PREFIX | ArgCommand::OVER, {.prefix = 1, .over = 1}},
      {true, ArgCommand::PREFIX | ArgCommand::OVER, {.prefix = 1}},
      {true, ArgCommand::PREFIX | ArgCommand::OVER, {.over = 1}},

      // Unspecified args ignored
      {true, ArgCommand::PREFIX | ArgCommand::FOCUS, {.prefix = 1, .over = 2}},
  };

  for (auto& [want_success, source, args] : test_cases) {
    const std::string description = absl::StrFormat("source %d", source);

    int save = 0;
    auto command = std::make_unique<ArgCommand>(
        'c', "help", source, ArgCommand::EXCLUSIVE, [&](int arg) {
          save = arg;
          return Command::EXEC_OK;
        });

    EXPECT_EQ(command->Execute({}), Command::USAGE) << description;

    const Command::ExecuteResult want_result =
        want_success ? Command::EXEC_OK : Command::USAGE;

    EXPECT_EQ(command->Execute(args), want_result) << description;
    if (want_success) {
      EXPECT_EQ(save, 1) << "source is " << source;
    }
  }
}

TEST(ClickCommandTest, Sequence) {
  cv::Point2i save_location;
  int save_arg = 0;
  auto command = std::make_unique<ClickCommand>(
      'c', "help", ClickCommand::PREFIX | ClickCommand::FOCUS,
      ArgCommand::EXCLUSIVE, [&](cv::Point2i location, int arg) {
        save_location = location;
        save_arg = arg;
        return Command::EXEC_OK;
      });

  CommandBuffer buf;
  buf.AddKey('c');

  EXPECT_EQ(command->Evaluate(buf), ClickCommand::NEED_CLICK);

  buf.AddClick();
  EXPECT_EQ(command->Evaluate(buf), ClickCommand::EVAL_OK);

  EXPECT_EQ(command->Execute({.mouse_coords = {10, 20}}), ClickCommand::USAGE);
  EXPECT_EQ(command->Execute({.prefix = 123, .mouse_coords = {10, 20}}),
            ClickCommand::EXEC_OK);

  EXPECT_THAT(save_location, CvPointEq(cv::Point2i(10, 20)));
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

}  // namespace
