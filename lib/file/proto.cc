#include "lib/file/proto.h"

#include <fcntl.h>

#include "absl/cleanup/cleanup.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

namespace {

class ErrorCollector : public google::protobuf::io::ErrorCollector {
 public:
  void AddError(int line, google::protobuf::io::ColumnNumber column,
                const std::string& message) override {
    errors.push_back(
        absl::StrFormat("%d:%d:%s", line + 1, column + 1, message));
  }

  std::vector<std::string> errors;
};

}  // namespace

absl::Status ReadProto(const std::string& path,
                       google::protobuf::Message* message) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    return absl::ErrnoToStatus(errno, "opening " + path);
  }

  google::protobuf::io::FileInputStream in(fd);
  in.SetCloseOnDelete(true);

  ErrorCollector error_collector;

  google::protobuf::TextFormat::Parser parser;
  parser.RecordErrorsTo(&error_collector);

  if (!parser.Parse(&in, message)) {
    return absl::UnknownError(
        absl::StrFormat("failed to read %s: %s", path,
                        absl::StrJoin(error_collector.errors, " / ")));
  }

  return absl::OkStatus();
}

absl::Status WriteTextProto(const std::string& path,
                            const google::protobuf::Message& message) {
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  if (fd < 0) {
    return absl::ErrnoToStatus(errno, "opening " + path);
  }

  google::protobuf::io::FileOutputStream out(fd);

  bool failed = false;
  int err = 0;

  if (!google::protobuf::TextFormat::Print(message, &out)) {
    failed = true;
    err = out.GetErrno();
  }
  if (!out.Close()) {
    if (!failed) {
      failed = true;
      err = out.GetErrno();
    }
  }

  if (failed) {
    absl::Status status;
    if (err == 0) {
      return absl::UnknownError("failed to write message");
    } else {
      return absl::ErrnoToStatus(err, "writing " + path);
    }
  }

  return absl::OkStatus();
}
