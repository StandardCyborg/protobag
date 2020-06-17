#pragma once

#include <memory>

#include "protobag/archive/Archive.hpp"
#include "protobag/Utils/Result.hpp"

namespace protobag {
namespace archive {

class Reader;
class Writer;

// Archive Impl: Using libarchive https://www.libarchive.org
class LibArchiveArchive final : public Archive {
public:

  /// Implementation of Archive Interface

  static Result<Archive::Ptr> Open(Archive::Spec s);
  static bool IsSupported(const std::string &format);
  
  virtual std::vector<std::string> GetNamelist() override;
  virtual Archive::ReadStatus ReadAsStr(const std::string &entryname) override;

  virtual OkOrErr Write(
    const std::string &entryname, const std::string &data) override;

  virtual std::string ToString() const override { 
    return std::string("LibArchiveArchive: ") + GetSpec().path;
  }


  /// Additional Utils; see ArchiveUtil.hpp for public API
  
  // Unpack `entryname` to the directory at `dest_dir` (and create any needed
  // sub-directories).  Use a "streaming" write so the entry is never entirely
  // in memory.
  OkOrErr StreamingUnpackEntryTo(
    const std::string &entryname,
    const std::string &dest_dir);
  
  // Add the file `src_file` to this archive with name `entryname`; use a
  // "streaming" read so that the file is never entirely in memory.
  OkOrErr StreamingAddFile(
    const std::string &src_file,
    const std::string &entryname);


private:
  friend class Reader;
  friend class Writer;
  class ImplBase;
  std::shared_ptr<ImplBase> _impl;
};

} /* namespace archive */
} /* namespace protobag */
