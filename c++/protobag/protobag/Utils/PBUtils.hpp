#pragma once

#include "protobag/Utils/Result.hpp"

#include <memory>
#include <fstream>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include <google/protobuf/any.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/time_util.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>


namespace google {
namespace protobuf {
class FileDescriptorSet;
} // protobuf
} // google


namespace protobag {


// See "PBFactory: Sugar" and other utils at end of file


// ============================================================================
// Utils ======================================================================
// ============================================================================

// Get the type URL for mesage type `MT`. Similar to 
// protobuf::interal::GetTypeUrl() except everything outside Google
// internal appears to default to the URL prefix "type.googleapis.com/"
// (kTypeGoogleApisComPrefix), so we just default to that.
template <typename MT>
inline std::string GetTypeURL() {
  return ::google::protobuf::internal::GetTypeUrl(
    MT::descriptor()->full_name(),
    ::google::protobuf::internal::kTypeGoogleApisComPrefix);
      // This prefix is what protobuf uses internally:
      // https://github.com/protocolbuffers/protobuf/blob/39d730dd96c81196893734ee1e075c34567e59ae/src/google/protobuf/any.cc#L44
}

// Similar to protobuf interal ParseTypeUrl(), except that we ignore the 
// leading url prefix, if any, because it's usually "type.googleapis.com/"
// (kTypeGoogleApisComPrefix).  The token returned is equivalent to the
// "full name" of a message (i.e. `MT::descriptor()->full_name()`).  See also:
// https://github.com/protocolbuffers/protobuf/blob/e492e5a4ef16f59010283befbde6112f1995fa0f/src/google/protobuf/util/type_resolver_util.cc#L308
inline std::string GetMessageTypeName(const std::string type_url) {
  size_t delim_pos = type_url.find("/");
  if (delim_pos == std::string::npos) {
    return type_url;
  } else {
    return type_url.substr(delim_pos + 1);
  }
}

inline ::google::protobuf::Duration SecondsToDuration(double d_seconds) {
  // For a good time: https://github.com/ros/roscpp_core/pull/50
  int64_t sec = floor(d_seconds);
  int32_t nanos = round((d_seconds - sec) * 1e9);
  sec += (nanos / 1000000000ul);
  nanos %= 1000000000ul;
  ::google::protobuf::Duration d;
  d.set_seconds(sec);
  d.set_nanos(nanos);
  return d;
}

inline ::google::protobuf::Timestamp SecondsToTimestamp(double t_seconds) {
  // For a good time: https://github.com/ros/roscpp_core/pull/50
  int64_t sec = floor(t_seconds);
  int32_t nanos = round((t_seconds - sec) * 1e9);
  sec += (nanos / 1000000000ul);
  nanos %= 1000000000ul;
  ::google::protobuf::Timestamp t;
  t.set_seconds(sec);
  t.set_nanos(nanos);
  return t;
}



// ============================================================================
// PBFactory ==================================================================
// ============================================================================

// PBFactory: factory I/O methods for protobuf messages <=> strings & other
// Based upon OarphKit https://github.com/pwais/oarphkit/blob/e799e7904d5b374cb6b58cd06a42d05506e83d94/oarphkit/ok/SerializationUtils/PBUtils-inl.hpp#L1
// TODO support protobuf arena allocation https://developers.google.com/protocol-buffers/docs/reference/arenas ?
class PBFactory {
public:

  // Deserialize ==============================================================

  template <typename MT>
  static Result<MT> UnpackFromAny(const ::google::protobuf::Any &any) {
    if (!any.Is<MT>()) {
      return {.error = fmt::format(
        "Any is not {} but is {}", GetTypeURL<MT>(), any.type_url())
      };
    }
    MT msg;
    bool success = any.UnpackTo(&msg);
    if (success) {
      return {.value = msg};
    }
    return {.error = fmt::format(
      "Failed to unpack a {} from Any[{}]", 
      GetTypeURL<MT>(), any.type_url())
    };
  }

  template <typename MT>
  static Result<MT> LoadFromPath(const std::string &path) {
    std::ifstream infile(path, std::ios::in | std::ios::binary);
    return LoadFromStream<MT>(infile);
  }

  // Load a message from `in` and return null on error
  template <typename MT>
  static Result<MT> LoadFromStream(
      std::istream &in,
      bool large_message_support=true) {

    if (in.good()) {
      return {.error = "Input stream in bad state"};
    }

    VerifyProfobuf();

    ///
    /// First, try reading Binary
    ///
    if (large_message_support) {
      MT message;
      google::protobuf::io::IstreamInputStream pb_iis(&in);
      auto res = LoadLargeFromPbInputStream(pb_iis, &message);
      if (res.IsOk()) { return {.value = std::move(message)}; }
    } else {
      // Use protobuf's built-in limits
      MT message;
      const bool success = message.ParseFromIStream(&in);
      if (success) { return {.value = std::move(message) }; }
    }

    ///
    /// Didn't read & return? Try TextFormat
    ///

    in.clear();
    in.seekg(0, std::ios::beg);
    if (in.good()) {
      return {.error = "Failed to rewind stream"};
    }

    std::stringstream ss;
    ss << in.rdbuf();
    MT message;
    const bool success =
      ::google::protobuf::TextFormat::ParseFromString(ss.str(), &message);
    if (success) { return {.value = message}; }

    return {
      .error = fmt::format("Failed to read a {}", GetTypeURL<MT>())
    };
  }

  template <typename MT>
  static Result<MT> LoadFromString(const std::string &text_format_str) {
    MT message;
    auto res = LoadTextFormatFromArray(
                (const std::byte *)text_format_str.data(),
                text_format_str.size(),
                &message);
    if (res.IsOk()) {
      return {.value = std::move(message)};
    } else {
      return {.error = res.error};
    }
  }

  template <typename MT, typename ContainerT>
  static Result<MT> LoadFromContainer(const ContainerT &c) {
    return LoadFromArray<MT>((const std::byte *) c.data(), c.size());
  }

  template <typename ContainerT>
  static OkOrErr LoadFromContainer(
                        const ContainerT &c,
                        ::google::protobuf::Message *message) {
    return LoadFromArray((const std::byte *) c.data(), c.size(), message);
  }

  template <typename MT>
  static Result<MT> LoadFromArray(const std::byte *data, size_t size) {
    MT message;
    auto res = LoadFromArray(data, size, &message);
    if (res.IsOk()) {
      return {.value = std::move(message)};
    } else {
      return {.error = res.error};
    }
  }
  
  static OkOrErr LoadFromArray(
                    const std::byte *data,
                    size_t size,
                    ::google::protobuf::Message *message) {
    
    if (!message) {
      return OkOrErr::Err(
        "Programming error: need user-allocated output message");
    }
    
    if ((data == nullptr) || (size == 0)) {
      return OkOrErr::Err("Bad array");
    }

    VerifyProfobuf();

    /// First, try reading Binary
    {
      google::protobuf::io::ArrayInputStream pb_ais(data, size);
      auto res = LoadLargeFromPbInputStream(pb_ais, message);
      if (res.IsOk()) { return kOK; }
    }

    /// Didn't read & return? Try TextFormat
    {
      auto res = LoadTextFormatFromArray(data, size, message);
      if (res.IsOk()) { return kOK; }
    }

    return OkOrErr::Err(
      fmt::format("Failed to read a {}", message->GetTypeName()));
  }



  // Serialize ================================================================

  template <typename MT>
  static Result<std::string> ToTextFormatString(
              const MT &m, bool newlines=true) {
    VerifyProfobuf();

    ::google::protobuf::TextFormat::Printer tfp;
    tfp.SetSingleLineMode(!newlines);

    std::string out;
    bool success = tfp.PrintToString(m, &out);
    if (!success) {
      return {.error = 
        fmt::format("Error trying to write a {} in Textformat",
        GetTypeURL<MT>())};
    } else {
      return {.value = out};
    }
  }

  template <typename MT>
  static Result<std::string> ToBinaryString(const MT &m) {
    VerifyProfobuf();
    std::string out;
    bool success = m.SerializeToString(&out);
    if (!success) {
      return {.error = 
        fmt::format("Error trying to write a {} in binary format",
        GetTypeURL<MT>())};
    } else {
      return {.value = out};
    }
  }

  template <typename MT>
  static OkOrErr SaveBinaryToPath(const MT &m, const std::string &path) {
    VerifyProfobuf();
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out.good()) {
      return {.error = fmt::format("Bad destination: {}", path)};
    }

    const bool write_success = m.SerializeToOstream(&out);
    if (!write_success) {
      return {.error =
        fmt::format("Error trying to write a {} in binary format to {}",
          GetTypeURL<MT>(),
          path)
      };
    }

    return kOK;
  }



protected:

  static void VerifyProfobuf() {
    // Throws a static assert if headers & linked library don't match.  Useful
    // when building for iOS, where Apple has its own libprotobuf build but
    // sometimes does not isolate it properly.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
  }

  template <typename PBInputStreamT>
  static OkOrErr LoadLargeFromPbInputStream(
                    PBInputStreamT &pb_iis,
                    ::google::protobuf::Message *message) {
    if (!message) {
      return OkOrErr::Err(
        "Programming error: need user-allocated output message");
    }

    try {
      /**
       * Support reading arbitrarily large messages. This feature is a
       * "security hazard" since an attacker could DOS/OOM the machine,
       * but in practice often necessary because protobuf only supports
       * 64MB messages by default.
       */
      google::protobuf::io::CodedInputStream cis(&pb_iis);
      cis.SetTotalBytesLimit(std::numeric_limits<int>::max());
        // Use all the RAM

      const bool success = message->ParseFromCodedStream(&cis);
      if (success) { return kOK; }
    } catch (std::exception &ex) {
      // These can be false positives b/c e.g. the message is in TextFormat
      return {.error = fmt::format(
        ("Exception while trying to read a {} protobuf message:\n {} \n"
         "(Skipping read-from-CodedInputStream)"),
        message->GetTypeName(),
        ex.what())
      };
    }

    return {.error = "Could not read large message"};
  }

  static OkOrErr LoadTextFormatFromArray(
                      const std::byte *data,
                      size_t size,
                      ::google::protobuf::Message *message) {
    if (!message) {
      return OkOrErr::Err(
        "Programming error: need user-allocated output message");
    }

    google::protobuf::io::ArrayInputStream pb_ais(data, size);
    const bool success = 
      ::google::protobuf::TextFormat::Parse(&pb_ais, message);
    if (success) {
      return kOK;
    } else {
      return {
        .error = fmt::format(
          "Failed to read a {} in text format from an array",
          message->GetTypeName())
      };
    }
  }

};



// ============================================================================
// PBFactory: Sugar ===========================================================
// ============================================================================

// Return a text format string, or throw on error.
template <typename MT>
std::string PBToString(const MT &pb_msg) {
  auto maybe_pb_txt = PBFactory::ToTextFormatString(pb_msg);
  if (!maybe_pb_txt.IsOk()) {
    throw std::runtime_error(maybe_pb_txt.error);
  }
  return *maybe_pb_txt.value;
}


// ============================================================================
// Reflection Utils ===========================================================
// ============================================================================

// Given a `message`, get the attribute at `fieldname`
Result<int32_t> GetAttr_int32(
    const ::google::protobuf::Message *message,
    const std::string &fieldname);
Result<int64_t> GetAttr_int64(
    const ::google::protobuf::Message *message,
    const std::string &fieldname);
Result<float> GetAttr_float(
    const ::google::protobuf::Message *message,
    const std::string &fieldname);
Result<double> GetAttr_double(
    const ::google::protobuf::Message *message,
    const std::string &fieldname);
Result<bool> GetAttr_bool(
    const ::google::protobuf::Message *message,
    const std::string &fieldname);
Result<std::string> GetAttr_string(
    const ::google::protobuf::Message *message,
    const std::string &fieldname);
Result<const ::google::protobuf::Message *> GetAttr_msg(
    const ::google::protobuf::Message *message,
    const std::string &fieldname);

// Given a `message`, get the attribute at `field_path`.  If `field_path` is
// a field on `message`, get that field.  Otherwise if `field_path` is a
// period (.) delimited string to a nested attribute, recursively descend
// into children of `message` to find the field.
Result<int32_t> GetDeep_int32(
    const ::google::protobuf::Message *message,
    const std::string &field_path);

Result<int64_t> GetDeep_int64(
    const ::google::protobuf::Message *message,
    const std::string &field_path);

Result<float> GetDeep_float(
    const ::google::protobuf::Message *message,
    const std::string &field_path);

Result<double> GetDeep_double(
    const ::google::protobuf::Message *message,
    const std::string &field_path);

Result<bool> GetDeep_bool(
    const ::google::protobuf::Message *message,
    const std::string &field_path);

Result<std::string> GetDeep_string(
    const ::google::protobuf::Message *message,
    const std::string &field_path);

// Special case: a field_path of "" returns the given message
Result<const ::google::protobuf::Message *> GetDeep_msg(
    const ::google::protobuf::Message *message,
    const std::string &field_path);

// TODO: support repeated, maps, enums, etc

const char * const GetPBCPPTypeName(
  ::google::protobuf::FieldDescriptor::CppType type_id);



// ============================================================================
// DynamicMessageFactory ======================================================
// ============================================================================

// Given a set of (serialized) protobuf Descriptors, `DynamicMsgFactory` will 
// help you decode messages using those descriptors **without** using protoc-
// generated headers and sources for those message types.  This utility wraps
// a ::google::protobuf::DynamicMessageFactory with a 
// ::google::protobuf::SimpleDescriptorDatabase in order to to help implement
// the "Self-Describing Message Technique" (w/out requiring that message def):
// https://developers.google.com/protocol-buffers/docs/techniques#self-description
// Protobuf has all the tools but only really puts them together in their 
// `util.json_util` module.
class DynamicMsgFactory {
public:
  typedef std::shared_ptr<DynamicMsgFactory> Ptr;

  typedef Result<std::unique_ptr<::google::protobuf::Message>> MsgPtrOrErr;

  // Create and return a Message (actually a DynamicMessage) of type `type_url`
  // from the given buffer.  From there, you can use the Message reflection
  // interface to get attributes of the Message instance; see:
  // https://github.com/protocolbuffers/protobuf/blob/e492e5a4ef16f59010283befbde6112f1995fa0f/src/google/protobuf/message.h#L79
  // See also the "Reflection Utils" section above.
  //                   !!!!! NOTE !!!!!
  // As per ::google::protobuf::DynamicMessageFactory, the returned
  // message has lifetime tied to this `DynamicMsgFactory` instance.  You
  // need to keep both in scope, or serialize and dump the message elsewhere.
  // To get your message's `type_url`, use either `GetTypeURL()` or 
  // `my_message_instance.GetDescriptor()->full_name()` will work too (we 
  // allow ignoring the url prefix / namespace, which is almost always 
  // "type.googleapis.com/"). See also:
  // https://github.com/protocolbuffers/protobuf/blob/e492e5a4ef16f59010283befbde6112f1995fa0f/src/google/protobuf/dynamic_message.cc#L632
  MsgPtrOrErr LoadFromArray(
                const std::string &type_url,
                const std::byte *data,
                size_t size);

  template <typename ContainerT>
  MsgPtrOrErr LoadFromContainer(
                  const std::string &type_url,
                  const ContainerT &c) {
    return LoadFromArray(type_url, (const std::byte *) c.data(), c.size());
  }

  // Register the given Protobuf type(s) with this factory by providing their
  // (serializable) message definition FileDescriptor(s)
  void RegisterTypes(const ::google::protobuf::FileDescriptorSet &fds);
  void RegisterType(const ::google::protobuf::FileDescriptorProto &fd);

  std::string ToString() const;

protected:
  struct Impl;
  std::shared_ptr<Impl> _impl;
  void LazyInitImpl();
};


} /* namespace protobag */
