/*
Copyright 2020 Standard Cyborg

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
