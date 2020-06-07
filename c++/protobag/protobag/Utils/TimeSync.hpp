#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "protobag/Entry.hpp"
#include "protobag/ReadSession.hpp"


namespace protobag {


// MaybeBundle is a bundle of N time-synchronized `Entry`s (or an error).  Has
// similar error state semantics as MaybeEntry.  Typically a MaybeBundle has
// one message per topic for a list of distinct topics requested from a 
// `TimeSync` below.
struct MaybeBundle : Result<std::list<Entry>> {
  static MaybeBundle EndOfSequence() { return Err("EndOfSequence"); }
  bool IsEndOfSequence() const { return error == "EndOfSequence"; }

  // See Archive::ReadStatus
  bool IsNotFound() const;

  static EntryBundle Err(const std::string &s) {
    EntryBundle m; m.error = s; return m;
  }

  static EntryBundle Ok(std::list<Entry> &&v) {
    EntryBundle m; m.value = std::move(v); return m;
  }
};

class TimeSync {
public:
  typedef std::shared_ptr<TimeSync> Ptr;
  virtual TimeSync() { }
  
  static Result<Ptr> Create(ReadSession::Ptr rs) {
    return {.error = "Base class does nothing"};
  }

  virtual MaybeBundle GetNext() {
    return MaybeBundle::EndOfSequence();
      // Base class has no data
  }

protected:
  ReadSession::Ptr _read_sess;
};


// Approximately synchronizes messages from given topics as follows:
//  * Waits until there is at least one StampedMessage for every topic
//  * Look at all possible bundlings of messages receieved thus far ...
//    * Discard any bundling with total time difference greater than 
//        `max_slop`
//    * Emit the bundle with minimal total time difference and dequeue emitted
//        messages
//    * Continue until source ReadSession exhausted
// Based upon ROS Python Approximate Time Sync (different from C++ version):
// https://github.com/ros/ros_comm/blob/c646e0f3a9a2d134c2550d2bf40b534611372662/utilities/message_filters/src/message_filters/__init__.py#L204
class MaxSlopTimeSync final : public TimeSync {
public:
  struct Spec {
    std::vector<std::string> topics;
    ::google::protobuf::Duration max_slop;
    size_t max_queue_size = 1;

    // static WithMaxSlop(float max_slop_sec) {
    //   Specs s;
    //   s.max_slop = SecondsToDuration(max_slop_sec);
    //   s.max_queue_size = size_t(-1);
    //   return s;
    // }
  };

  static Result<TimeSync::Ptr> Create(
    const ReadSession::Ptr &rs,
    const Spec &spec);
  
  MaybeBundle GetNext() override;

protected:
  Spec _spec;

  struct Impl;
  std::shared_ptr<Impl> _impl;
};


} /* namespace protobag */
