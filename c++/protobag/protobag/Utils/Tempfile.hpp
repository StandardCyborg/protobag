#pragma once

#include <filesystem>
#include <string>

#include "protobag/Utils/Result.hpp"

namespace protobag {

Result<std::filesystem::path> CreateTempfile(
  const std::string &suffix="",
  size_t max_attempts=100);

Result<std::filesystem::path> CreateTempdir(
  const std::string &suffix="",
  size_t max_attempts=100);

} /* namespace protobag */