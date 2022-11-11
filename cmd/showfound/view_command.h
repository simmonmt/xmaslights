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

constexpr int kLeftMouseButton = -1;

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

  Command(int key, const std::string& help) : key_(key), help_(help) {}
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

  struct Trigger {
    std::string qualifiers;
    std::string key;
    bool click_required = false;
  };

  virtual Trigger DescribeTrigger() const { return {.key = KeyToString(key_)}; }

  ExecuteResult Execute(const Args& args) const;

 protected:
  virtual bool ArgsAreValid(const Args& args) const = 0;
  virtual ExecuteResult CallFunc(const Args& args) const = 0;

 private:
  int key_;
  std::string help_;
};

std::ostream& operator<<(std::ostream& os, Command::EvaluateResult result);
std::ostream& operator<<(std::ostream& os, Command::ExecuteResult result);
std::ostream& operator<<(std::ostream& os, Command::Trigger trigger);

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
  BareCommand(int key, const std::string& usage,
              std::function<ExecuteResult()> func)
      : Command(key, usage), func_(func) {}
  ~BareCommand() override = default;

 private:
  bool ArgsAreValid(const Args& args) const override {
    return args.prefix.has_value() == false;
  }

  ExecuteResult CallFunc(const Args& args) const override { return func_(); }

  std::function<ExecuteResult()> func_;
};

namespace internal {

class ArgCommandBase : public Command {
 public:
  enum ArgSource {
    PREFIX = 0x1,  // prefix always wins if present
    FOCUS = 0x2,
    OVER = 0x4,
  };

  enum ArgMode {
    PREFER = 0x1,     // allow multiple sources with precedence order
    EXCLUSIVE = 0x2,  // allow only one source
  };

  ArgCommandBase(int key, const std::string& usage, unsigned int arg_source,
                 ArgMode arg_mode, bool click_required);
  ~ArgCommandBase() override = default;

  Trigger DescribeTrigger() const override;

  EvaluateResult Evaluate(const CommandBuffer& buf) const override;

 protected:
  bool ArgsAreValid(const Args& args) const override;
  int ArgFromArgs(const Args& args) const;

  const unsigned int arg_source_;
  const ArgMode arg_mode_;
  const bool click_required_;
};

}  // namespace internal

class ArgCommand : public internal::ArgCommandBase {
 public:
  ArgCommand(int key, const std::string& usage, unsigned int arg_source,
             ArgMode arg_mode, std::function<ExecuteResult(int)> func)
      : ArgCommandBase(key, usage, arg_source, arg_mode, false), func_(func) {}
  ~ArgCommand() override = default;

 private:
  ExecuteResult CallFunc(const Args& args) const override {
    return func_(ArgFromArgs(args));
  }

  const std::function<ExecuteResult(int)> func_;
};

class ClickCommand : public internal::ArgCommandBase {
 public:
  ClickCommand(int key, const std::string& usage, unsigned int arg_source,
               ArgMode arg_mode,
               std::function<ExecuteResult(cv::Point2i, int)> func)
      : ArgCommandBase(key, usage, arg_source, arg_mode, true), func_(func) {}
  ~ClickCommand() override = default;

 private:
  ExecuteResult CallFunc(const Args& args) const override {
    return func_(args.mouse_coords, ArgFromArgs(args));
  }

  const std::function<ExecuteResult(cv::Point2i, int)> func_;
};

#endif  // _CMD_SHOWFOUND_VIEW_COMMAND_H_
