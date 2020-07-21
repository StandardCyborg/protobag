#pragma once

#include <optional>
#include <string>

#include <fmt/format.h>

#include <google/protobuf/any.h>
#include <google/protobuf/any.pb.h>

#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/Result.hpp"
#include "protobag_msg/ProtobagMsg.pb.h"


namespace protobag {


struct Entry;
struct MaybeEntry;
std::string GetTopicFromEntryname(const std::string &entryname);


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


  // == Optional context, mainly for timeseries and indexing features =========
  struct Context {
    /// Time Series Data; StampedMessage

    // For timeseries data: the topic, which is a directory for a sequence
    // of messages that all have the same type.
    std::string topic;
    
    // For timeseries data: the time associated with a message.
    ::google::protobuf::Timestamp stamp;

    // For timeseries data: the type URI of the contained message (if known)
    std::string inner_type_url;
   

    /// Descriptor Indexing

    // For descriptor indexing, which allows readers of your protobag to decode
    // messages without having your protobuf definitions.  This is the
    // descriptor of the innermost msg (not StampedMessage nor Any).
    // Mainly for writing; for reading we can't populate this member.
    const ::google::protobuf::Descriptor *descriptor = nullptr;

    // For non-C++ writers (e.g. Python), part of Descriptor Indexing needs
    // to run in the writer's language where the Protobuf message definitions
    // are available.  Those writers may supply a read-only reference to a
    // completed `FileDescriptorSet`-- something we can't construct in the C++
    // side of the write codepath.
    const ::google::protobuf::FileDescriptorSet *fds = nullptr;
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
      // Intentionally leave type_url empty; it means the message is raw
    
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

  // Create a StampedMessage entry given the user-provided `msg_bytes` and 
  // `type_url`; are `msg_bytes` actually from a message of type `type_url`?
  // We leave that issue "Unchecked" and trust the user.
  // This API is typically not useful to end users but is used internally for
  // write support (e.g. python `protobag_native`).
  static Entry CreateStampedUnchecked(
        const std::string &topic,
        uint64_t sec,
        uint32_t nsec,
        const std::string &type_url,
        std::string &&msg_bytes,
        const ::google::protobuf::FileDescriptorSet *fds=nullptr,
        const ::google::protobuf::Descriptor *descriptor=nullptr) {

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
                .fds = fds,
                .descriptor = descriptor,
              });

  }
  
  
  // == Reading & Accessing ===================================================

  template <typename MT>
  bool IsA() const {
    return msg.type_url() == GetTypeURL<MT>();
  }

  bool IsStampedMessage() const {
    return 
      IsA<StampedMessage>() || (
        // An unpacked StampedDatum is OK too
        GetTopicTime().has_value());
  }

  bool IsRaw() const {
    return msg.type_url().empty();
  }

  std::optional<TopicTime> GetTopicTime() const;

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

  MaybeEntry UnpackFromStamped() const;

  // == Other Utils ===========================================================
  
  std::string ToString() const;

  // bool operator==(const Entry &other) const;
  bool EntryDataEqualTo(const Entry &other) const {
    return
      entryname == other.entryname &&
      msg.type_url() == other.msg.type_url() &&
      msg.value() == other.msg.value();
  }

};


// A Result<Entry> with a reserved "error" state for end of a stream of
// entries; similar to python `StopIteration`.
struct MaybeEntry : public Result<Entry> {
  static MaybeEntry EndOfSequence() { return Err("EndOfSequence"); }
  bool IsEndOfSequence() const { return error == "EndOfSequence"; }

  // See Archive::ReadStatus for definition; this can be an acceptible error
  bool IsNotFound() const;

  static MaybeEntry Err(const std::string &s) {
    MaybeEntry m; m.error = s; return m;
  }

  static MaybeEntry Ok(Entry &&v) {
    MaybeEntry m; m.value = std::move(v); return m;
  }
};


inline MaybeEntry Entry::UnpackFromStamped() const {
  if (!IsStampedMessage()) {
    return MaybeEntry::Err(fmt::format(
      "Entry is not actually a StampedMessage. Entry: {}", ToString()));
  }

  auto maybe_stamped = PBFactory::UnpackFromAny<StampedMessage>(msg);
  if (!maybe_stamped.IsOk()) {
    return MaybeEntry::Err(fmt::format(
      "Failed to decode StampedMessage: {} .  Entry: {}",
      maybe_stamped.error, ToString()));
  }

  const StampedMessage &stamped = *maybe_stamped.value;
  Entry entry = {
    .entryname = entryname,
    .ctx = Context{
      .topic = GetTopicFromEntryname(entryname),
      .stamp = stamped.timestamp(),
      .inner_type_url = stamped.msg().type_url(),
    }
  };
  entry.msg.set_type_url(stamped.msg().type_url());
  entry.msg.set_value(std::move(stamped.msg().value()));
  return MaybeEntry::Ok(std::move(entry));
}

inline std::optional<TopicTime> Entry::GetTopicTime() const {
  if (ctx.has_value()) {
    TopicTime tt;
    tt.set_topic(ctx->topic);
    *tt.mutable_timestamp() = ctx->stamp;
    return tt;
  } else if (IsA<StampedMessage>()) {
    const auto &maybebUnpacked = UnpackFromStamped();
    if (maybebUnpacked.IsOk()) {
      return maybebUnpacked.value->GetTopicTime();
    } else {
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }
}

} /* namespace protobag */
