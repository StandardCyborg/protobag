/*
Copyright 2020 Standard Cyborg

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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