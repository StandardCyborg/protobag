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

struct Entry {
  // == Core Entry ============================================================

  // The name of this entry; similar to a file path relative to the root of
  // an archive.
  std::string entryname;
    // Never empty on read.  If empty on write, then Protobag will auto-
    // generate an entryname (likely based on `context.topic` below).

  // The payload of the entry.  
  ::google::protobuf::Any msg;
    // If `type_url` is unset, then the message is a "raw message" and Protobag
    // will forgo all indexing and boxing; see CreateRaw() below.  But raw mode
    // is a special case and `type_url`, is usually automatically set in
    // the Entry factory methods.

  // Optional context, mainly for writing
  struct Context {
    // For timeseries data: the topic, which is a directory for a sequence
    // of messages that all have the same type.
    std::string topic;
    
    // For timeseries data: the time associated with a message.
    ::google::protobuf::Timestamp stamp;

    // For timeseries data: the type URI of the contained message (if known)
    std::string inner_type_url;

    // For descriptor indexing, which allows readers of your protobag to decode
    // messages without having your protobuf definitions.  This is the
    // descriptor of the innermost msg (not StampedMessage nor Any).
    const ::google::protobuf::Descriptor *descriptor = nullptr;
  };
  std::optional<Context> ctx;

  
  // == Writing & Creating ====================================================

  // == Common Messages =========================

  template <typename MT>
  static Entry Create(
        const std::string &entryname,
        const MT &msg) {

    return Create<MT>(
      entryname,
      msg,
      {
        .descriptor = msg.GetDescriptor(),
        .inner_type_url = GetTypeURL<MT>(),
      });
  }

  template <typename MT>
  static Entry Create(
        const std::string &entryname,
        const MT &msg,
        const Context &ctx) {

    ::google::protobuf::Any packed;
    packed.PackFrom(msg);
    return {
      .entryname = entryname,
      .msg = packed,
      .ctx = ctx,
    };
  }

  static Entry CreateUnchecked(
        const std::string &entryname,
        const std::string &type_url,
        std::string &&msg_bytes,
        const Context &ctx) {

    // Below: essentially equivalent to PackFrom(), but the type_url might not
    // be equivalent.  FMI:
    // https://github.com/protocolbuffers/protobuf/blob/39d730dd96c81196893734ee1e075c34567e59ae/src/google/protobuf/any.cc#L48
    ::google::protobuf::Any packed;
    packed.set_type_url(type_url);
    packed.set_value(std::move(msg_bytes));

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
      maybe_encoded = PBFactory::ToTextFormatString(raw_msg);
    } else {
      maybe_encoded = PBFactory::ToBinaryString(raw_msg);
    }

    if (!maybe_encoded.IsOk()) {
      return {.error = maybe_encoded.error};
    }

    return {.value = 
      CreateRawFromBytes(entryname, std::move(*maybe_encoded.value))
    };
  }

  static Entry CreateRawFromBytes(
        const std::string &entryname,
        std::string &&raw_msg_contents) {

    ::google::protobuf::Any packed;
    packed.set_value(std::move(raw_msg_contents));
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
                .inner_type_url = GetTypeURL<MT>(),
                .descriptor = msg.GetDescriptor(),
              });

  }

  static Entry CreateStampedUnchecked(
        const std::string &topic,
        uint64_t sec,
        uint32_t nsec,
        const std::string &type_url,
        std::string &&msg_bytes) {
          // TODO: add descriptor-like context; see protobag_native.cpp ~~~~~~~~~~~~~~~~

    StampedMessage stamped_msg;
    stamped_msg.mutable_timestamp()->set_seconds(sec);
    stamped_msg.mutable_timestamp()->set_nanos(nsec);
    
    // Below: essentially equivalent to PackFrom(), but the type_url might not
    // be equivalent.  FMI:
    // https://github.com/protocolbuffers/protobuf/blob/39d730dd96c81196893734ee1e075c34567e59ae/src/google/protobuf/any.cc#L48
    stamped_msg.mutable_msg()->set_type_url(type_url);
    stamped_msg.mutable_msg()->set_value(std::move(msg_bytes));
    
    return Create(
              "", 
              stamped_msg,
              {
                .topic = topic,
                .stamp = stamped_msg.timestamp(),
                .inner_type_url = type_url,
                // .descriptor = msg.GetDescriptor(), TODO
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

  bool IsRaw() const {
    return msg.type_url().empty();
  }

  const std::string &GetTopic() const {
    if (ctx.has_value()) {
      return ctx->topic;
    } else {
      static const std::string kEmpty;
      return kEmpty;
    }
  }

  template <typename MT>
  Result<MT> GetAs(bool validate_type_url = true) const {
    if (validate_type_url) {
      if (IsRaw()) {
        return {.error = fmt::format((
          "Tried to decode a {} but this entry has no known type_url. "
          "Try again with validation disabled; you will also need to "
          "accept that you might be casting the wrong protocol upon "
          "this buffer.  Entry: {}"
          ), GetTypeURL<MT>(), ToString())
        };
      } else if (msg.type_url() != GetTypeURL<MT>()) {
        return {.error = fmt::format(
          "Tried to read a {} but entry is a {}.  Entry: {}",
          GetTypeURL<MT>(), msg.type_url(), ToString())
        };
      }
    }

    return PBFactory::UnpackFromAny<MT>(msg);
  }


  // == Other Utils ===========================================================
  
  std::string ToString() const;

  // bool operator==(const Entry &other) const;
  bool EntryDataEqualTo(const Entry &other) const {
    return
      entryname == other.entryname &&
      msg.type_url() == other.msg.type_url() &&
      msg.value() == other.msg.value();
  }

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
  static MaybeEntry EndOfSequence() { return Err("EndOfSequence"); }
  bool IsEndOfSequence() const { return error == "EndOfSequence"; }

  // See Archive::ReadStatus
  bool IsNotFound() const;

  static MaybeEntry Err(const std::string &s) {
    MaybeEntry m; m.error = s; return m;
  }

  static MaybeEntry Ok(Entry &&v) {
    MaybeEntry m; m.value = std::move(v); return m;
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
