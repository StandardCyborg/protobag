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