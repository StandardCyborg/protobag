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
  Archive::Spec final_spec = s;
  if (final_spec.format.empty()) {
    final_spec.format = InferFormat(s.path);
  }

  if (final_spec.format == "memory") {
    if (final_spec.memory_archive) {
      return {.value = final_spec.memory_archive};
    } else {
      return MemoryArchive::Open(final_spec);
    }
  } else if (final_spec.format == "directory") {
    return DirectoryArchive::Open(final_spec);
  } else if (LibArchiveArchive::IsSupported(final_spec.format)) {
    return LibArchiveArchive::Open(final_spec);
  } else if (final_spec.format.empty()) {
    return {
      .error=fmt::format("Could not infer format for {}", final_spec.path)
    };
  } else {
    return {
      .error=fmt::format(
        "Unsupported format {} for {}", final_spec.format, final_spec.path)
    };
  }
}

} /* namespace archive */
} /* namespace protobag */
