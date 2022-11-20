#include "cmd/showfound/view_command.h"

#include <ctype.h>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"

std::string KeyToString(int key) {
  if (isprint(key)) {
    return absl::StrFormat("%c", key);
  } else if (key == kLeftArrowKey) {
    return "<-";
  } else if (key == kRightArrowKey) {
    return "->";
  } else if (key == kEscapeKey) {
    return "ESC";
  } else if (key == ' ') {
    return "SPC";
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
  return CallFunc(args);
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
    if (buf.clicked()) {
      if (auto iter = keys_.find(kLeftMouseButton); iter != keys_.end()) {
        return iter->second.get();
      }
    }

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
    Command::Trigger trigger = command.DescribeTrigger();
    std::string trigger_str =
        absl::StrFormat("%s %s %s", trigger.qualifiers, trigger.key,
                        trigger.click_required ? "*" : " ");
    out.emplace_back(trigger_str, command.help());
    max_trigger_width = std::max(max_trigger_width, trigger_str.size());
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

std::ostream& operator<<(std::ostream& os, Command::Trigger trigger) {
  return os << absl::StreamFormat("%s %s %s", trigger.qualifiers, trigger.key,
                                  trigger.click_required ? "<click>" : "");
}

namespace internal {

ArgCommandBase::ArgCommandBase(int key, const std::string& usage,
                               unsigned int arg_source, ArgMode arg_mode,
                               bool click_required)
    : Command(key, usage),
      arg_source_(arg_source),
      arg_mode_(arg_mode),
      click_required_(click_required) {
  QCHECK_NE(arg_source, 0UL) << "no sources specified";
}

ArgCommandBase::Trigger ArgCommandBase::DescribeTrigger() const {
  std::vector<std::string> qualifiers;
  if (arg_source_ & PREFIX) {
    qualifiers.push_back("prefix");
  }
  if (arg_source_ & FOCUS) {
    qualifiers.push_back("focus");
  }
  if (arg_source_ & OVER) {
    qualifiers.push_back("over");
  }

  const std::string separator = arg_mode_ == PREFER ? ">" : "|";

  return {
      .qualifiers = absl::StrJoin(qualifiers, separator),
      .key = (key() < 0 ? " " : KeyToString(key())),
      .click_required = click_required_,
  };
}

ArgCommandBase::EvaluateResult ArgCommandBase::Evaluate(
    const CommandBuffer& buf) const {
  if (click_required_) {
    return buf.clicked() ? EVAL_OK : NEED_CLICK;
  }
  return EVAL_OK;
}

bool ArgCommandBase::ArgsAreValid(const Args& args) const {
  int num_true = 0;

  if ((arg_source_ & PREFIX) && args.prefix.has_value()) {
    num_true++;
  }
  if ((arg_source_ & FOCUS) && args.focus.has_value()) {
    num_true++;
  }
  if ((arg_source_ & OVER) && args.over.has_value()) {
    num_true++;
  }

  // Evaluate checked for any required click

  if (arg_mode_ == PREFER) {
    return num_true > 0;
  }
  return num_true == 1;  // exclusive
}

int ArgCommandBase::ArgFromArgs(const Args& args) const {
  if ((arg_source_ & PREFIX) && args.prefix.has_value()) {
    return *args.prefix;
  }
  if ((arg_source_ & FOCUS) && args.focus.has_value()) {
    return *args.focus;
  }
  if ((arg_source_ & OVER) && args.over.has_value()) {
    return *args.over;
  }
  return 0;
}

}  // namespace internal
