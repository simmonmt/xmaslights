#include "cmd/showfound/view_command.h"

#include <ctype.h>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"

std::string KeyToString(int key) {
  if (isprint(key)) {
    return absl::StrFormat("%c", key);
  } else if (key == kLeftArrowKey) {
    return "<-";
  } else if (key == kRightArrowKey) {
    return "->";
  } else if (key == kEscapeKey) {
    return "ESC";
  }
  return absl::StrFormat("%d", key);
}

// TODO: Rewrite as state machine
CommandBuffer::CommandBuffer() : clicked_(false), error_(NONE) {}

void CommandBuffer::Reset() { ResetAll(); }

void CommandBuffer::ResetAll() {
  prefix_.reset();
  key_.reset();
  clicked_ = false;
  error_ = NONE;
}

void CommandBuffer::ResetKey() {
  key_.reset();
  clicked_ = false;
  error_ = NONE;
}

void CommandBuffer::AddKey(int key) {
  error_ = NONE;

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
  error_ = NONE;

  if (clicked_) {
    ResetAll();
  }
  clicked_ = true;
}

void CommandBuffer::SetError(Error error) {
  ResetAll();
  error_ = error;
}

std::string CommandBuffer::ToString() {
  switch (error_) {
    case UNKNOWN:
      return "Err: UNKNOWN CMD";
    case USAGE:
      return "Err: USAGE";
    case NONE:
      break;
  }

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

Command::ExecuteResult Command::Execute(const Args& args) const {
  if (!ArgsAreValid(args)) {
    return USAGE;
  }
  return func_(args);
}

Keymap::Keymap() {
  Add(std::make_unique<BareCommand>('?', "lists registered commands", [&] {
    Dump(std::cout);
    return Command::EXEC_OK;
  }));
}

void Keymap::Add(std::unique_ptr<Command> command) {
  int key = command->key();
  bool found = std::get<1>(keys_.emplace(command->key(), std::move(command)));
  QCHECK(found) << "duplicate key in keymap: " << KeyToString(key);
}

std::variant<Keymap::LookupResult, const Command*> Keymap::Lookup(
    const CommandBuffer& buf) const {
  if (!buf.key().has_value()) {
    return CONTINUE;
  }

  if (auto iter = keys_.find(*buf.key()); iter != keys_.end()) {
    return iter->second.get();
  }

  return UNKNOWN;
}

void Keymap::Dump(std::ostream& os) const {
  std::vector<std::tuple<std::string, std::string>> out;
  unsigned long max_trigger_width = 0;

  std::vector<int> sorted_keys;
  for (const auto& [key, unused] : keys_) {
    sorted_keys.push_back(key);
  }
  std::sort(sorted_keys.begin(), sorted_keys.end());

  for (const auto& key : sorted_keys) {
    const Command& command = *keys_.find(key)->second;
    std::string trigger = command.DescribeTrigger();
    out.emplace_back(trigger, command.help());
    max_trigger_width = std::max(max_trigger_width, trigger.size());
  }

  for (const auto& [key, help] : out) {
    os << absl::StreamFormat("%*s %s\n", max_trigger_width, key, help);
  }
}

std::ostream& operator<<(std::ostream& os, Command::EvaluateResult result) {
  switch (result) {
    case Command::NEED_CLICK:
      return os << "NEED_CLICK";
    case Command::EVAL_OK:
      return os << "OK";
  }
  return os << "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, Command::ExecuteResult result) {
  switch (result) {
    case Command::USAGE:
      return os << "USAGE";
    case Command::ERROR:
      return os << "ERROR";
    case Command::EXEC_OK:
      return os << "OK";
  }
  return os << "UNKNOWN";
}
