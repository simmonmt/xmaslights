#ifndef _LIB_TESTING_PROTO_H_
#define _LIB_TESTING_PROTO_H_ 1

#include <string>

#include "absl/log/check.h"
#include "absl/strings/ascii.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"

template <class T>
T ParseTextProtoOrDie(const std::string& txt) {
  T pb;
  QCHECK(google::protobuf::TextFormat::ParseFromString(txt, &pb));
  return pb;
}

template <class T>
bool ProtoDiff(const T& want, const T& got, std::string* diffs) {
  diffs->clear();
  google::protobuf::util::MessageDifferencer differencer;
  differencer.ReportDifferencesToString(diffs);
  return differencer.Compare(want, got);
}

#endif  // _LIB_TESTING_PROTO_H_
