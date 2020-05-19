#pragma once

#include <memory>
#include <string>
#include <vector>

#include "protobag/Utils/Result.hpp"

namespace protobag {
namespace archive {

// An interface abstracting away the archive 
class Archive {
public:
  typedef std::shared_ptr<Archive> Ptr;
  virtual ~Archive() { Close(); }
  
  // Opening an archive for reading / writing
  struct Spec {
    // clang-format off
    std::string mode;
      // Choices: "read", "write" (which also means append)
    std::string path;
      // TODO a path ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // Special values:
      //   "<tempfile>" - Generate a 
    std::string format;
      // Choices:
      //   "directory" - Simply use an on-disk directory as an "archive". Does
      //     not require a 3rd party back-end.
    // clang-format on
    static Spec WriteToTempdir() {
      return {
        .mode = "write",
        .path = "<tempfile>",
        .format = "directory",
      };
    }
  };
  static Result<Ptr> Open(const Spec &s=Spec::WriteToTempdir());
  virtual void Close() { }

  // Reading
  virtual std::vector<std::string> GetNamelist() { return {}; }
  virtual Result<std::string> ReadAsStr(const std::string &entryname) {
    return Result<std::string>::Err("Reading unsupported in base");
  }

  // Writing
  virtual OkOrErr Write(
    const std::string &entryname, const std::string &data) {
      return OkOrErr::Err("Writing unsupported in base");
  }

  // Properties
  virtual const Spec &GetSpec() const { return _spec; }
  virtual std::string ToString() const { return "Base"; }

protected:
  Archive() { }
  Archive(const Archive&) = delete;
  Archive& operator=(const Archive&) = delete;

  Spec _spec;
};

} /* namespace archive */
} /* namespace protobag */
