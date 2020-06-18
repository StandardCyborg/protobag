#include "protobag/archive/Archive.hpp"

#include <filesystem>

#include <fmt/format.h>

#include "protobag/archive/DirectoryArchive.hpp"
#include "protobag/archive/LibArchiveArchive.hpp"
#include "protobag/archive/MemoryArchive.hpp"
#include "protobag/ArchiveUtil.hpp"

namespace fs = std::filesystem;

namespace protobag {
namespace archive {

inline bool EndsWith(const std::string &s, const std::string &suffix) {
  return (s.size() >= suffix.size()) && (
    s.substr(s.size() - suffix.size(), suffix.size()) == suffix);
}

std::string InferFormat(const std::string &path) {
  auto maybeDir = IsDirectory(path);
  if (maybeDir.IsOk() && *maybeDir.value) {
    return "directory";
  } else {

    // TODO: support more extensions
    std::vector<std::string> exts = {"zip", "tar"};
    for (auto &ext : exts) {
      if (EndsWith(path, ext)) {
        return ext;
      }
    }
  }

  return "";
}

Result<Archive::Ptr> Archive::Open(const Archive::Spec &s) {
  std::string format = s.format;
  if (format.empty()) {
    format = InferFormat(s.path);
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
  } else if (format.empty()) {
    return {
      .error=fmt::format("Could not infer format for {}", s.path)
    };
  } else {
    return {
      .error=fmt::format("Unsupported format {} for {}", format, s.path)
    };
  }
}

} /* namespace archive */
} /* namespace protobag */
