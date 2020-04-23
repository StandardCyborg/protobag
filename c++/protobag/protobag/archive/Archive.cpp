#include "protobag/archive/Archive.hpp"

#include <filesystem>

#include <fmt/format.h>

#include "protobag/archive/DirectoryArchive.hpp"
#include "protobag/archive/LibArchiveArchive.hpp"


// #if PROTOBAG_HAVE_LIBARCHIVE



// #else /* PROTOBAG_HAVE_LIBARCHIVE is false */
//   #error "Currently we only support LibArchive"
// #endif

namespace fs = std::filesystem;

namespace protobag {
namespace archive {

inline bool EndsWith(const std::string &s, const std::string &suffix) {
  return (s.size() >= suffix.size()) && (
    s.substr(s.size() - suffix.size(), suffix.size()) == suffix);
}

Result<Archive::Ptr> Archive::Open(const Archive::Spec &s) {
  std::string format = s.format;
  if (format.empty()) {
    if (fs::is_directory(s.path)) {
      format = "directory";
    } else {
      // TODO make this better ... ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      std::vector<std::string> exts = {"zip", "tar"};
      for (auto &ext : exts) {
        if (EndsWith(s.path, ext)) {
          format = ext;
          break;
        }
      }
      format = format.empty() ? "directory" : format;
    }
  }

  if (format == "directory") {
    return DirectoryArchive::Open(s);
  } else if (LibArchiveArchive::IsSupported(format)) {
    return LibArchiveArchive::Open(s);
  }

  return {.error=fmt::format("Unsupported format {}", s.format)};
}

} /* namespace archive */
} /* namespace protobag */
