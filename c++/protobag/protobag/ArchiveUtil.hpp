#pragma once

#include <string>
#include <vector>

#include "protobag/archive/LibArchiveArchive.hpp"
#include "protobag/Utils/Result.hpp"

namespace protobag {

// This module contains a set of utilties for manipulating raw archives
// (e.g. Zip and Tar files) using LibArchive (through Protobag's
// `LibArchiveArchive` wrapper).  We include them because some users (e.g.
// iOS and Emscripten) might lack an archive utility.  Rather than including
// both Protobag and some other util (like iOS ZipArchive), you can use
// the utilities provided here.


// Expand the contents of `archive_path` to `dest_dir`; we'll create
// `dest_dir` if it does not already exist.  Does not delete `archive_path`.
OkOrErr UnpackArchiveToDir(
  const std::string &archive_path,
  const std::string &dest_dir);

// Create a new archive at `destination` from the given files `file_list`.
// Either guess the format from the extention suffix of `destination`
// or forcibly use `format`, which could be "zip", "tar", etc.
// Optionally compute archive entry names relative to `base_dir`.
OkOrErr CreateArchiveAtPath(
  const std::vector<std::string> &file_list,
  const std::string &destination,
  const std::string &format="",
  const std::string &base_dir="");

// Like `CreateArchiveAtPath()`, except we scan `src_dir` recursively and
// use that for our `file_list`.  (Ignores symlinks, empty directories, etc;
// includes only `is_regular_file()` entries).
OkOrErr CreateArchiveAtPathFromFiles(
  const std::string &src_dir,
  const std::string &destination,
  const std::string &format="");

// Get all regular files in `dir`
Result<std::vector<std::string>> GetAllFilesRecursive(const std::string &dir);

// Return true if `path` exists and is a directory, or return an error
Result<bool> IsDirectory(const std::string &path);

// Read the file at `path` into a string using the C++ Filesystem
// POSIX-backed API.  On error, return "".
std::string ReadFile(const std::string &path);

} /* namespace protobag */
