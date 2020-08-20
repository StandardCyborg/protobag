#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "protobag/Entry.hpp"
#include "protobag/ReadSession.hpp"

namespace protobag {

typedef std::list<Entry> EntryBundle;

// MaybeBundle is a bundle of N time-synchronized `Entry`s (or an error).  Has
// similar error state semantics as MaybeEntry.  Typically a MaybeBundle has
// one message per topic for a list of distinct topics requested from a 
// `TimeSync` below.
struct MaybeBundle : Result<EntryBundle> {
  static MaybeBundle EndOfSequence() { return Err("EndOfSequence"); }
  bool IsEndOfSequence() const { return error == "EndOfSequence"; }

  // See Archive::ReadStatus
  bool IsNotFound() const;

  static MaybeBundle Err(const std::string &s) {
    MaybeBundle m; m.error = s; return m;
  }

  static MaybeBundle Ok(EntryBundle &&v) {
    MaybeBundle m; m.value = std::move(v); return m;
  }
};

// Base interace to a Time Synchronization algorithm.  
class TimeSync {
public:
  typedef std::shared_ptr<TimeSync> Ptr;
  virtual ~TimeSync() { }
  
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
//  * Waits until there is at least one StampedMessage for every topic (and 
//      ignores entries that lack topci/timestamp data)
//  * Look at all possible bundlings of messages receieved thus far ...
//    * Discard any bundle with total time difference greater than `max_slop`
//    * Emit the bundle with minimal total time difference and dequeue emitted
//        messages
//    * Continue until source ReadSession exhausted
// Useful for:
//  * synchronizing topic recorded at different rates-- the closest match will
//      be emitted each time
//  * robustness to dropped messages-- this utility will queue up to
//      `max_queue_size` messages per topic, so if one or more synchronized
//      topics has a missing message (or two, or three..), bundles for those
//      missing messages will be skipped, but other bundles with full data
//      will be retained.
//
// NOTE: for each bundle of messages emitted, uses 
//      O( 2^|topics * (max_queue_size - 1)| ) time, 
//   since the algorithm examines all possible bundlings.  In pratice,
//   this operation is plenty fast as long as you have no more than 5-10
//   topics and keep `max_queue_size` of 5-ish.  See test
//   `IterProductsTest.Test7PoolsSize5`, which takes about ~16ms on a
//   modern Xeon.
//
// Based upon ROS Python Approximate Time Sync (different from C++ version):
// https://github.com/ros/ros_comm/blob/c646e0f3a9a2d134c2550d2bf40b534611372662/utilities/message_filters/src/message_filters/__init__.py#L204
class MaxSlopTimeSync final : public TimeSync {
public:
  struct Spec {
    std::vector<std::string> topics;
    ::google::protobuf::Duration max_slop;
    size_t max_queue_size = 1; // Recall: max queue size *per topic*

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
