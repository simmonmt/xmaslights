#ifndef _CMD_SHOWFOUND_VIEW_COMMAND_H_
#define _CMD_SHOWFOUND_VIEW_COMMAND_H_ 1

#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include "opencv2/core/types.hpp"

constexpr int kLeftArrowKey = 2;
constexpr int kRightArrowKey = 3;
constexpr int kEscapeKey = 27;

class CommandBuffer {
 public:
  CommandBuffer();
  ~CommandBuffer() = default;

  std::optional<int> prefix() const { return prefix_; }
  std::optional<int> key() const { return key_; }
  bool clicked() const { return clicked_; }

  void AddKey(int key);
  void AddClick();
  void Reset();
  std::string ToString();

 private:
  void ResetAll();
  void ResetKey();

  std::optional<int> prefix_;
  std::optional<int> key_;
  bool clicked_;
};

class Keymap {
 public:
  Keymap();

  void AddKey(int key, const std::string& usage, std::function<void()> func);
  void AddReqPrefixKey(int key, const std::string& usage,
                       std::function<void(int)> func);

  void Print(std::ostream& os) const;

  enum Result {
    UNKNOWN,
    ERROR,
    EXECUTED,
    NEED_MOUSE,
  };
  Result Execute(const CommandBuffer& buf, std::optional<int> focus,
                 cv::Point2i mouse) const;

  std::string Usage(const CommandBuffer& buf) const;

 private:
  enum Requirements {
    NONE,
    REQ_PREFIX,
  };

  void CheckNoDuplicateKey(int key);
  std::string FormatUsageKey(int key, Requirements req) const;

  std::map<int, std::tuple<std::string, Requirements>> usages_;
  std::unordered_map<
      int, std::function<Result(const CommandBuffer&, std::optional<int>,
                                cv::Point2i mouse)>>
      keys_;
};

std::ostream& operator<<(std::ostream& os, const Keymap::Result result);

#endif  // _CMD_SHOWFOUND_VIEW_COMMAND_H_
