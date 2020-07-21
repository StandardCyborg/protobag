#pragma once

#include <string>

#include <tuple> 
#include <google/protobuf/util/time_util.h>

#include "protobag_msg/ProtobagMsg.pb.h"

namespace protobag {


inline bool EntryIsInTopic(
    const std::string &entryname,
    const std::string &topic) {
  return entryname.find(topic) == 0;
}

inline bool IsProtoBagIndexTopic(const std::string &topic) {
  return EntryIsInTopic(topic, "/_protobag_index");
}

inline
bool operator<(const TopicTime &tt1, const TopicTime &tt2) {
  return
    std::make_tuple(tt1.timestamp(), tt1.topic(), tt1.entryname()) <
    std::make_tuple(tt2.timestamp(), tt2.topic(), tt2.entryname());
}

inline
bool operator>(const TopicTime &tt1, const TopicTime &tt2) {
  return
    std::make_tuple(tt1.timestamp(), tt1.topic(), tt1.entryname()) >
    std::make_tuple(tt2.timestamp(), tt2.topic(), tt2.entryname());
}

inline ::google::protobuf::Timestamp MinTimestamp() {
  ::google::protobuf::Timestamp t;
  t.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMinSeconds);
  t.set_nanos(0);
  return t;
}

inline ::google::protobuf::Timestamp MaxTimestamp() {
  ::google::protobuf::Timestamp t;
  t.set_seconds(::google::protobuf::util::TimeUtil::kTimestampMaxSeconds);
  t.set_nanos(0);
  return t;
}

} /* namespace protobag */