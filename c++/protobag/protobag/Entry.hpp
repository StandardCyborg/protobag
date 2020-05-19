#pragma once

#include <string>

#include "protobag/Utils/Result.hpp"

#include "protobag_msg/ProtobagMsg.pb.h"

// TODO maybe move with TopicTime operator< below
#include <tuple> 
#include <google/protobuf/util/time_util.h>


namespace protobag {

struct Entry {
  std::string topic;
  StampedMessage stamped_msg;

  template <typename MT>
  static Entry Create(
    const std::string &topic,
    const ::google::protobuf::Timestamp &t,
    const MT &msg) {

      StampedMessage stamped_msg;
      stamped_msg.mutable_msg()->PackFrom(msg);
      *stamped_msg.mutable_timestamp() = t;

      return {.topic = topic, .stamped_msg = stamped_msg};
  }

  template <typename MT>
  static Entry Create(
    const std::string &topic,
    uint64_t sec, uint32_t nsec,
    const MT &msg) {

    ::google::protobuf::Timestamp t;
    t.set_seconds(sec);
    t.set_nanos(nsec);
    return Entry::Create(topic, t, msg);

  }

  template <typename MT>
  static Entry Create(const std::string &topic, uint64_t sec, const MT &msg) {
    return Create(topic, sec, 0, msg);
  }
  
  bool operator<(const Entry &other) const {
    return (topic < other.topic) ||
      (stamped_msg.timestamp().seconds() < other.stamped_msg.timestamp().seconds()) ||
      (stamped_msg.timestamp().nanos() < other.stamped_msg.timestamp().nanos());
  }
};

// A Result<Entry> with a reserved "error" state for end of a stream of
// entries; similar to python `StopIteration`.
struct MaybeEntry : public Result<Entry> {
  static MaybeEntry EndOfSequence() {
    MaybeEntry m;
    m.error = "EndOfSequence";
    return m;
  }
  
  bool IsEndOfSequence() { return error == "EndOfSequence"; }

  // FIXME can't we brace init for the base class ..? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  static MaybeEntry Err(const std::string &s) {
    MaybeEntry m;
    m.error = s;
    return m;
  }

  static MaybeEntry Ok(Entry &&v) {
    MaybeEntry m;
    m.value = std::move(v);
    return m;
  }
};



// TODO move these to core protobag ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
inline bool EntryIsInTopic(const std::string &entryname, const std::string &topic) {
  return entryname.find(topic) == 0;
}

inline bool IsProtobagMetaTopic(const std::string &topic) {
  return EntryIsInTopic(topic, "/_protobag_meta");
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


} /* namespace protobag */
