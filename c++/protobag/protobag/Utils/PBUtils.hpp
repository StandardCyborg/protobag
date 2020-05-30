#pragma once

#include "protobag/Utils/Result.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include <google/protobuf/any.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>



namespace protobag {

// See "Syntactic Sugar" and other utils at end of file


// ============================================================================
// Utils ======================================================================
// ============================================================================

// Get the type URL for mesage type `MT`. Similar to 
// protobuf::interal::GetTypeUrl() except everything outside Google
// internal appears to default to the URL prefix `type.googleapis.com/`
// (kTypeGoogleApisComPrefix), so we just default to that.
template <typename MT>
inline std::string GetTypeURL() {
  return ::google::protobuf::internal::GetTypeUrl(
    MT::descriptor()->full_name(),
    ::google::protobuf::internal::kTypeGoogleApisComPrefix);
      // This prefix is what protobuf uses internally:
      // https://github.com/protocolbuffers/protobuf/blob/39d730dd96c81196893734ee1e075c34567e59ae/src/google/protobuf/any.cc#L44
}


// ============================================================================
// PBFactory: =================================================================
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
      google::protobuf::io::IstreamInputStream pb_iis(&in);
      auto maybe_message = LoadLargeFromPbInputStream<MT>(pb_iis);
      if (maybe_message.IsOk()) { return maybe_message; }
    } else {
      // Use protobuf's built-in limits
      MT message;
      const bool success = message.ParseFromIStream(&in);
      if (success) { return {.value = message }; }
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
    return LoadTextFormatFromArray<MT>(
              (const std::byte *)text_format_str.data(),
              text_format_str.size());
  }

  template <typename MT, typename ContainerT>
  static Result<MT> LoadFromContainer(const ContainerT &c) {
    return LoadFromArray<MT>((const std::byte *) c.data(), c.size());
  }

  template <typename MT>
  static Result<MT> LoadFromArray(const std::byte *data, size_t size) {
    if ((data == nullptr) || (size == 0)) {
      return {.error = "Bad array"};
    }

    VerifyProfobuf();

    /// First, try reading Binary
    {
      google::protobuf::io::ArrayInputStream pb_ais(data, size);
      Result<MT> maybe_msg = LoadLargeFromPbInputStream<MT>(pb_ais);
      if (maybe_msg.IsOk()) { return maybe_msg; }
    }

    /// Didn't read & return? Try TextFormat
    {
      Result<MT> maybe_msg = LoadTextFormatFromArray<MT>(data, size);
      if (maybe_msg.IsOk()) { return maybe_msg; }
    }

    return {
      .error = fmt::format("Failed to read a {}", GetTypeURL<MT>())
    };
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

  template <typename MT, typename PBInputStreamT>
  static Result<MT> LoadLargeFromPbInputStream(PBInputStreamT &pb_iis) {
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

      MT message;
      const bool success = message.ParseFromCodedStream(&cis);
      if (success) {
        return {.value = message};
      }
    } catch (std::exception &ex) {
      // These can be false positives b/c e.g. the message is in TextFormat
      return {.error = fmt::format(
        "Exception while trying to read protobuf message:\n {} \n(Skipping read-from-CodedInputStream)",
        ex.what())
      };
    }

    return {.error = "Could not read large message"};
  }

  template <typename MT>
  static Result<MT> LoadTextFormatFromArray(const std::byte *data, size_t size) {
    MT message;
    google::protobuf::io::ArrayInputStream pb_ais(data, size);
    const bool success = ::google::protobuf::TextFormat::Parse(&pb_ais, &message);
    if (success) {
      return {.value = message};
    } else {
      return {
        .error = fmt::format(
          "Failed to read a {} in text format from an array",
          GetTypeURL<MT>())
      };
    }
  }

};



// ============================================================================
// Sugar ======================================================================
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


} /* namespace protobag */
