#ifndef _CMD_SHOWFOUND_VIEW_COMMAND_H_
#define _CMD_SHOWFOUND_VIEW_COMMAND_H_ 1

#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include "lib/base/base.h"
#include "opencv2/core/types.hpp"

constexpr int kLeftArrowKey = 2;
constexpr int kRightArrowKey = 3;
constexpr int kEscapeKey = 27;

std::string KeyToString(int key);

class CommandBuffer {
 public:
  CommandBuffer();
  ~CommandBuffer() = default;

  enum Error {
    NONE,
    UNKNOWN,  // Unknown command
    USAGE,    // Incorrectly-invoked command
  };

  std::optional<int> prefix() const { return prefix_; }
  std::optional<int> key() const { return key_; }
  bool clicked() const { return clicked_; }
  Error error() const { return error_; }

  void AddKey(int key);
  void AddClick();

  void SetError(Error error);

  void Reset();
  std::string ToString();

 private:
  void ResetAll();
  void ResetKey();

  std::optional<int> prefix_;
  std::optional<int> key_;
  bool clicked_;
  Error error_;

  DISALLOW_COPY_AND_ASSIGN(CommandBuffer);
};

class Command {
 public:
  struct Args {
    std::optional<int> prefix;
    std::optional<int> focus;
    std::optional<int> over;
    cv::Point2i mouse_coords;
  };

  enum ExecuteResult {
    USAGE,
    ERROR,
    EXEC_OK,
  };

  typedef std::function<ExecuteResult(const Args&)> Func;

  Command(int key, std::string help, Func func)
      : key_(key), help_(help), func_(func) {}
  virtual ~Command() = default;

  int key() const { return key_; }
  std::string help() const { return help_; };

  enum EvaluateResult {
    NEED_CLICK,
    EVAL_OK,
  };

  // Determine whether anything else is needed in the CommandBuffer
  // before this command can be executed.
  virtual EvaluateResult Evaluate(const CommandBuffer& buf) const {
    return EVAL_OK;
  }

  virtual std::string DescribeTrigger() const { return KeyToString(key_); }

  ExecuteResult Execute(const Args& args) const;

 protected:
  virtual bool ArgsAreValid(const Args& args) const = 0;

 private:
  int key_;
  std::string help_;
  Func func_;
};

std::ostream& operator<<(std::ostream& os, Command::EvaluateResult result);
std::ostream& operator<<(std::ostream& os, Command::ExecuteResult result);

class Keymap {
 public:
  Keymap();

  void Add(std::unique_ptr<Command> command);

  enum LookupResult {
    UNKNOWN,
    CONTINUE,
  };

  std::variant<LookupResult, const Command*> Lookup(
      const CommandBuffer& buf) const;

  void Dump(std::ostream& os) const;

 private:
  std::unordered_map<int, std::unique_ptr<Command>> keys_;

  DISALLOW_COPY_AND_ASSIGN(Keymap);
};

class BareCommand : public Command {
 public:
  BareCommand(int key, std::string usage, std::function<ExecuteResult()> func)
      : Command(key, usage, [func](const Args&) { return func(); }) {}
  ~BareCommand() override = default;

 private:
  bool ArgsAreValid(const Args& args) const override {
    return args.prefix.has_value() == false;
  }
};

class PrefixCommand : public Command {
 public:
  PrefixCommand(int key, std::string usage,
                std::function<ExecuteResult(int)> func)
      : Command(key, usage,
                [func](const Args& args) { return func(*args.prefix); }) {}
  ~PrefixCommand() override = default;

  std::string DescribeTrigger() const override {
    return "prefix " + KeyToString(key());
  }

 private:
  bool ArgsAreValid(const Args& args) const override {
    return args.prefix.has_value();
  }
};

class OverUnlessPrefixCommand : public Command {
 public:
  OverUnlessPrefixCommand(int key, std::string usage,
                          std::function<ExecuteResult(int)> func)
      : Command(key, usage, [func](const Args& args) {
          return func(args.prefix.has_value() ? *args.prefix : *args.over);
        }) {}
  ~OverUnlessPrefixCommand() override = default;

  std::string DescribeTrigger() const override {
    return "over|prefix " + KeyToString(key());
  }

 private:
  bool ArgsAreValid(const Args& args) const override {
    return args.prefix.has_value() || args.over.has_value();
  }
};

#endif  // _CMD_SHOWFOUND_VIEW_COMMAND_H_