#include "cmd/showfound/view_command.h"

#include <ctype.h>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"

CommandBuffer::CommandBuffer() : clicked_(false) {}

void CommandBuffer::Reset() { ResetAll(); }

void CommandBuffer::ResetAll() {
  prefix_.reset();
  key_.reset();
  clicked_ = false;
}

void CommandBuffer::ResetKey() {
  key_.reset();
  clicked_ = false;
}

void CommandBuffer::AddKey(int key) {
  if (key >= '0' && key <= '9') {
    if (key_.has_value()) {
      ResetAll();
    }

    if (!prefix_.has_value()) {
      prefix_ = 0;
    }
    prefix_ = *prefix_ * 10 + (key - '0');
    return;
  }

  if (key_.has_value()) {
    ResetAll();
  }

  key_ = key;
}

void CommandBuffer::AddClick() {
  if (clicked_) {
    ResetAll();
  }
  clicked_ = true;
}

namespace {

std::string KeyToString(int key) {
  if (isprint(key)) {
    return absl::StrFormat("%c", key);
  } else if (key == kLeftArrowKey) {
    return "<-";
  } else if (key == kRightArrowKey) {
    return "->";
  }
  return absl::StrFormat("%d", key);
}

}  // namespace

std::string CommandBuffer::ToString() {
  std::string out;
  if (prefix_.has_value()) {
    absl::StrAppendFormat(&out, "%d", *prefix_);
  }
  if (key_.has_value()) {
    if (!out.empty()) {
      out += " ";
    }
    out += KeyToString(*key_);
  }
  return out;
}

Keymap::Keymap() {
  AddKey('?', "lists registered commands", [&] { Print(std::cout); });
}

void Keymap::AddKey(int key, const std::string& usage,
                    std::function<void()> func) {
  CheckNoDuplicateKey(key);

  usages_[key] = std::make_tuple(usage, NONE);
  keys_[key] = [func](const CommandBuffer& buf, std::optional<int> focus,
                      cv::Point2i mouse) {
    if (buf.prefix().has_value()) {
      return ERROR;
    }
    func();
    return EXECUTED;
  };
}

void Keymap::AddReqPrefixKey(int key, const std::string& usage,
                             std::function<void(int)> func) {
  CheckNoDuplicateKey(key);

  usages_[key] = std::make_tuple(usage, REQ_PREFIX);
  keys_[key] = [func](const CommandBuffer& buf, std::optional<int> focus,
                      cv::Point2i mouse) {
    std::optional<int> prefix = buf.prefix();
    if (!prefix.has_value()) {
      return ERROR;
    }

    func(*prefix);
    return EXECUTED;
  };
}

void Keymap::CheckNoDuplicateKey(int key) {
  QCHECK(usages_.find(key) == usages_.end())
      << "duplicate key in keymap: " << KeyToString(key);
}

Keymap::Result Keymap::Execute(const CommandBuffer& buf,
                               std::optional<int> focus,
                               cv::Point2i mouse) const {
  std::optional<int> key = buf.key();
  if (!key.has_value()) {
    return ERROR;
  }

  auto iter = keys_.find(*key);
  if (iter == keys_.end()) {
    return UNKNOWN;
  }

  return iter->second(buf, focus, mouse);
}

std::string Keymap::FormatUsageKey(int key, Requirements req) const {
  return KeyToString(key);
}

std::string Keymap::Usage(const CommandBuffer& buf) const {
  std::optional<int> key = buf.key();
  if (!key.has_value()) {
    return "No command specified";
  }

  auto iter = usages_.find(*key);
  if (iter == usages_.end()) {
    return absl::StrFormat("No registered command for key %s",
                           KeyToString(*key));
  }

  auto& [usage, req] = iter->second;
  return absl::StrFormat("%s %s", FormatUsageKey(*key, req), usage);
}

void Keymap::Print(std::ostream& os) const {
  std::vector<std::tuple<std::string, std::string>> out;
  unsigned long max_key_width = 0;

  for (auto& iter : usages_) {
    const int key = iter.first;
    auto& [usage, req] = iter.second;

    std::string usage_key = FormatUsageKey(key, req);
    out.emplace_back(usage_key, usage);
    max_key_width = std::max(max_key_width, usage_key.size());
  }

  for (const auto& [key, usage] : out) {
    os << absl::StreamFormat("%*s %s\n", max_key_width, key, usage);
  }
}

std::ostream& operator<<(std::ostream& os, const Keymap::Result result) {
  switch (result) {
    case Keymap::UNKNOWN:
      return os << "UNKNOWN";
    case Keymap::ERROR:
      return os << "ERROR";
    case Keymap::EXECUTED:
      return os << "EXECUTED";
    case Keymap::NEED_MOUSE:
      return os << "NEED_MOUSE";
  }
}
