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

#include "protobag/ArchiveUtil.hpp"

#include <filesystem>

#include <fmt/format.h>

#include "protobag/archive/Archive.hpp"
#include "protobag/archive/LibArchiveArchive.hpp"


namespace fs = std::filesystem;

namespace protobag {

// NB: http://0x80.pl/notesen/2019-01-07-cpp-read-file.html
// We use C++ Filesystem POSIX-backed API because it's the fastest
std::string ReadFile(const std::string &path) {
  std::FILE* f = std::fopen(path.c_str(), "r");
  if (!f) {
    return "";
  }

  const auto f_size = fs::file_size(path);
  std::string res;
  res.resize(f_size);

  std::fread(&res[0], 1, f_size, f);

  std::fclose(f);
  return res;
}

Result<bool> IsDirectory(const std::string &path) {
  std::error_code err;
  bool is_dir = fs::exists(path, err) && fs::is_directory(path, err);
  if (err) {
    return {.error = fmt::format("{}: {}", path, err.message())};
  } else{
    return {.value = is_dir};
  }
}

Result<std::vector<std::string>> GetAllFilesRecursive(const std::string &dir) {
  auto maybeIsDir = IsDirectory(dir);
  if (!maybeIsDir.IsOk()) {
    return {.error = maybeIsDir.error};
  } else if (!*maybeIsDir.value) {
    return {.error = fmt::format("{} is not a directory", dir)};
  }

  std::vector<std::string> files;
  {
    std::error_code err;
    auto it = fs::recursive_directory_iterator(dir, err);
    if (err) { 
      return {.error = 
        fmt::format("Failed to get list for {}: {}", dir, err.message())
      };
    }

    for (const auto &entry : it) {
      if (fs::is_regular_file(entry, err)) {
        files.push_back(fs::absolute(entry));
      }

      if (err) {
        return {.error = fmt::format(
          "Could not get status for path {} in {}",
          entry.path().u8string(),
          dir)
        };
      }
    }
  }

  return {.value = files};
}


OkOrErr UnpackArchiveToDir(
    const std::string &archive_path,
    const std::string &dest_dir) {


  // Open the archive
  auto maybeAr = archive::LibArchiveArchive::Open({
    .mode = "read",
    .path = archive_path,
  });
  if (!maybeAr.IsOk()) { return OkOrErr::Err(maybeAr.error); }

  archive::LibArchiveArchive *reader = 
    dynamic_cast<archive::LibArchiveArchive *>(maybeAr.value->get());
  if (!reader) {
    return OkOrErr::Err(
      fmt::format(
        "Programming error: could not get libarchive api for {}",
        archive_path));
  }

  // Create dest
  std::error_code err;
  if (!fs::exists(dest_dir, err)) {
    if (err) { return {.error = err.message()}; }

    fs::create_directories(dest_dir, err);
    if (err) {
      return {.error = fmt::format(
        "Could not create root directory {}: {}", dest_dir, err.message())
      };
    }
  }

  // Unpack
  for (const auto &entryname : reader->GetNamelist()) {
    auto status = reader->StreamingUnpackEntryTo(
      entryname,
      dest_dir);
    if (!status.IsOk()) {
      return {.error = fmt::format(
        "Failed to unpack entry {} to {}: {}",
        entryname, dest_dir, status.error)
      };
    }
  }

  return kOK;
}

OkOrErr CreateArchiveAtPath(
    const std::vector<std::string> &file_list,
    const std::string &destination,
    const std::string &format,
    const std::string &base_dir) {

  std::string inferred_format = format;
  if (inferred_format.empty()) {
    inferred_format = archive::InferFormat(destination);
    if (inferred_format.empty()) {
      return OkOrErr::Err(
        fmt::format("Could not infer format for {}", destination));
    }
  }

  auto maybeWriter = archive::LibArchiveArchive::Open({
    .mode = "write",
    .path = destination,
    .format = inferred_format,
  });
  if (!maybeWriter.IsOk()) {
    return OkOrErr::Err(
      fmt::format(
        "Failed to create archive at {}: {}", destination, maybeWriter.error));
  }
  auto writerP = *maybeWriter.value;

  archive::LibArchiveArchive *writer = 
    dynamic_cast<archive::LibArchiveArchive *>(writerP.get());
  if (!writer) {
    return OkOrErr::Err(
      fmt::format(
        "Programming error: could not get libarchive api for {}",
        destination));
  }

  for (const auto &path : file_list) {
    std::string entryname = path;
    if (!base_dir.empty()) {
      entryname = fs::relative(path, base_dir);
    }

    auto status = writer->StreamingAddFile(path, entryname);
    if (!status.IsOk()) {
      return OkOrErr::Err(
        fmt::format(
          "Failed to write {} to {} as entry {}. Leaving {} as-is. Error: {}",
          path,
          destination,
          entryname,
          destination,
          status.error));
    }
  }

  return kOK;
}

OkOrErr CreateArchiveAtPathFromDir(
    const std::string &src_dir,
    const std::string &destination,
    const std::string &format) {


  auto maybeFiles = GetAllFilesRecursive(src_dir);
  if (!maybeFiles.IsOk()) {
    return OkOrErr::Err(
      fmt::format(
        "Failed to create archive at {}: {}", destination, maybeFiles.error));
  } else {
    return CreateArchiveAtPath(
              *maybeFiles.value,
              destination,
              format,
              src_dir);
  }
}

} /* namespace protobag */