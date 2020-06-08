#include "protobag/archive/Archive.hpp"

#include <filesystem>

#include <fmt/format.h>

#include "protobag/archive/DirectoryArchive.hpp"
#include "protobag/archive/LibArchiveArchive.hpp"
#include "protobag/archive/MemoryArchive.hpp"

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
      // TODO: support more extensions
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

  if (format == "memory") {
    if (s.memory_archive) {
      return {.value = s.memory_archive};
    } else {
      return MemoryArchive::Open(s);
    }
  } else if (format == "directory") {
    return DirectoryArchive::Open(s);
  } else if (LibArchiveArchive::IsSupported(format)) {
    return LibArchiveArchive::Open(s);
  }

  return {.error=fmt::format("Unsupported format {}", s.format)};
}

} /* namespace archive */
} /* namespace protobag */
