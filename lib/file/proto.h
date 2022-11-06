#ifndef _LIB_FILE_PROTO_H_
#define _LIB_FILE_PROTO_H_ 1

#include "absl/status/status.h"
#include "google/protobuf/message.h"

absl::Status ReadProto(const std::string& path,
                       google::protobuf::Message* message);
absl::Status WriteTextProto(const std::string& path,
                            const google::protobuf::Message& message);

#endif  // _LIB_FILE_PROTO_H_
