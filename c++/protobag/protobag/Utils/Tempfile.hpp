#pragma once

#include <filesystem>
#include <string>

#include "protobag/Utils/Result.hpp"

namespace protobag {

// Create an empty temp file in the canonical temp directory and return
// the path; the filename may have the given `suffix`
Result<std::filesystem::path> CreateTempfile(
  const std::string &suffix="",
  size_t max_attempts=100);

// Create an empty temp directory nested inside the canonical temp directory.
// The directory has a random name, perhaps with the given `suffix`.
Result<std::filesystem::path> CreateTempdir(
  const std::string &suffix="",
  size_t max_attempts=100);

} /* namespace protobag */