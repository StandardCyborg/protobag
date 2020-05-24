#pragma once

#include "protobag/archive/Archive.hpp"

namespace protobag {
namespace archive {

// Archive Impl: A fake "archive" that is simply a directory on local disk
class DirectoryArchive final : public Archive {
public:
  static Result<Archive::Ptr> Open(Archive::Spec s);
  
  virtual std::vector<std::string> GetNamelist() override;
  virtual Archive::ReadStatus ReadAsStr(const std::string &entryname) override;

  virtual OkOrErr Write(
    const std::string &entryname, const std::string &data) override;

  virtual std::string ToString() const override { 
    return std::string("DirectoryArchive: ") + GetSpec().path;
  }
};

} /* namespace archive */
} /* namespace protobag */
