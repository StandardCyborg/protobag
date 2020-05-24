#pragma once

#include <memory>

#include "protobag/archive/Archive.hpp"
#include "protobag/Utils/Result.hpp"

namespace protobag {
namespace archive {

// Archive Impl: Using libarchive https://www.libarchive.org
class LibArchiveArchive final : public Archive {
public:
  static Result<Archive::Ptr> Open(Archive::Spec s);
  static bool IsSupported(const std::string &format);
  
  virtual std::vector<std::string> GetNamelist() override;
  virtual Archive::ReadStatus ReadAsStr(const std::string &entryname) override;

  virtual OkOrErr Write(
    const std::string &entryname, const std::string &data) override;

  virtual std::string ToString() const override { 
    return std::string("LibArchiveArchive: ") + GetSpec().path;
  }

  class ImplBase;

private:
  std::shared_ptr<ImplBase> _impl;

  // struct impl;
  // struct reader;
  // struct writer;

  // typedef std::shared_ptr<impl> implptr;

  // struct libarchive_data;
  // std::shared_ptr<libarchive_data> _lib;

  // std::shared_ptr<libarchive_data> GetReadable();
};

} /* namespace archive */
} /* namespace protobag */
