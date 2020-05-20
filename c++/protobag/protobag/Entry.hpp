#pragma once

#include <optional>
#include <string>

#include <fmt/format.h>

#include <google/protobuf/any.h>
#include <google/protobuf/any.pb.h>

#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/Result.hpp"
#include "protobag_msg/ProtobagMsg.pb.h"

// TODO maybe move with TopicTime operator< below
#include <tuple> 
#include <google/protobuf/util/time_util.h>


namespace protobag {


// TODO move to pbfactory
template <typename MT>
inline std::string GetTypeURL() {
  return ::google::protobuf::internal::GetTypeUrl(
    T::descriptor()->full_name(),
    ::google::protobuf::internal::kTypeGoogleApisComPrefix);
      // This prefix is what protobuf uses internally:
      // https://github.com/protocolbuffers/protobuf/blob/39d730dd96c81196893734ee1e075c34567e59ae/src/google/protobuf/any.cc#L44
}

struct Entry {
  // == Core Entry ============================================================

  std::string entryname;
    // Never empty on read.  If empty on write, then Protobag will auto-
    // generate an entryname (likely based on `context.topic` below).

  ::google::protobuf::Any msg;
    // If `type_url` is unset, then the message is a "raw message" and Protobag
    // will forgo all indexing; see CreateRaw() below.  But `type_url`, is
    // usually automatically set.

  struct Context {
    std::string topic;
    ::google::protobuf::Timestamp stamp;
    const ::google::protobuf::Descriptor *descriptor = nullptr;
  };
  std::optional<Context> ctx;

  
  // == Writing & Creating ====================================================

  // == Common Messages =========================

  template <typename MT>
  static Entry Create(
        const std::string &entryname,
        const MT &msg) {

    return Create<MT>(entryname, msg, {.descriptor = msg.GetDescriptor()});
  }

  template <typename MT>
  static Entry Create(
        const std::string &entryname,
        const MT &msg,
        const Context &ctx) {

    ::google::protobuf::Any packed;
    packed.PackFrom(raw_msg);
    return {
      .entryname = entryname,
      .msg = packed,
      .ctx = ctx,
    };
  }


  // == Raw Mode ================================

  template <typename MT>
  static Result<Entry> CreateRaw(
        const std::string &entryname,
        const MT &raw_msg,
        bool use_text_format = false) {
    
    Result<std::string> maybe_encoded;
    if (use_text_format) {
      maybe_encoded = PBFactory<MT>::ToTextFormatString(raw_msg);
    } else {
      maybe_encoded = PBFactory<MT>::ToBinaryString(raw_msg);
    }

    if (!maybe_encoded.IsOk()) {
      return {.error = maybe_encoded.error};
    }

    return {.value = CreateRaw(entryname, *maybe_encoded.value)};
  }

  static Entry CreateRaw(
        const std::string &entryname,
        const std::string &raw_msg_contents) {

    ::google::protobuf::Any packed;
    *packed.mutable_value() = raw_msg_contents;
      // Intentionally leave type_url empty
    
    return {
      .entryname = entryname,
      .msg = packed,
    };
  }


  // == Time Series Data ========================

  template <typename MT>
  static Entry CreateStamped(
        const std::string &topic,
        uint64_t sec, uint32_t nsec,
        const MT &msg) {
    
    ::google::protobuf::Timestamp t;
    t.set_seconds(sec);
    t.set_nanos(nsec);
    return CreateStamped(topic, t, msg);
  }

  template <typename MT>
  static Entry CreateStamped(
        const std::string &topic,
        const ::google::protobuf::Timestamp &t,
        const MT &msg) {

    StampedMessage stamped_msg;
    stamped_msg.mutable_msg()->PackFrom(msg);
    *stamped_msg.mutable_timestamp() = t;

    return Create(
              "", 
              stamped_msg,
              {
                .topic = topic,
                .stamp = t,
                .descriptor = msg.GetDescriptor(),
              });

  }
  
  
  // == Reading & Accessing ===================================================

  template <typename MT>
  bool IsA() const {
    return msg.type_url() == GetTypeURL<MT>();
  }

  bool IsStampedMessage() const {
    return IsA<StampedMessage>();
  }

  template <typename MT>
  Result<MT> GetAs(bool validate_type_url = true) const {
    if (validate_type_url) {
      if (msg.type_url().empty()) {
        return {.error = fmt::format((
          "Tried to decode a {} but this entry has no known type_url. "
          "Try again with validation disabled; you will also need to "
          "accept that you might be casting the wrong protocol upon "
          "this buffer.  Entry: {}"
          ), GetTypeURL<MT>(), ToString());
        };
      } else if (msg.type_url() != GetTypeURL<MT>()) {
        return {.error = fmt::format(
          "Tried to read a {} but entry is a {}.  Entry: {}",
          GetTypeURL<MT>(), msg.type_url(), ToString())
        )};
      }
    }

    return PBFactory<MT>::UnpackFromAny(msg);
  }


  // == Other Utils ===========================================================
  
  std::string ToString() const;

  // bool operator<(const Entry &other) const {
  //   return (topic < other.topic) ||
  //     (stamped_msg.timestamp().seconds() < other.stamped_msg.timestamp().seconds()) ||
  //     (stamped_msg.timestamp().nanos() < other.stamped_msg.timestamp().nanos());
  // }

};


// struct Entry {
//   std::string topic;
//   StampedMessage stamped_msg;

//   template <typename MT>
//   static Entry Create(
//     const std::string &topic,
//     const ::google::protobuf::Timestamp &t,
//     const MT &msg) {

//       StampedMessage stamped_msg;
//       stamped_msg.mutable_msg()->PackFrom(msg);
//       *stamped_msg.mutable_timestamp() = t;

//       return {.topic = topic, .stamped_msg = stamped_msg};
//   }

//   template <typename MT>
//   static Entry Create(
//     const std::string &topic,
//     uint64_t sec, uint32_t nsec,
//     const MT &msg) {

//     ::google::protobuf::Timestamp t;
//     t.set_seconds(sec);
//     t.set_nanos(nsec);
//     return Entry::Create(topic, t, msg);

//   }

//   template <typename MT>
//   static Entry Create(const std::string &topic, uint64_t sec, const MT &msg) {
//     return Create(topic, sec, 0, msg);
//   }
  
//   bool operator<(const Entry &other) const {
//     return (topic < other.topic) ||
//       (stamped_msg.timestamp().seconds() < other.stamped_msg.timestamp().seconds()) ||
//       (stamped_msg.timestamp().nanos() < other.stamped_msg.timestamp().nanos());
//   }
// };

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


} /* namespace protobag */
