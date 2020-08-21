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

#include <optional>
#include <string>

namespace protobag {

// A hacky std::expected<> while the committee seeks consensus
template <typename T>
struct Result {
  std::optional<T> value;
  std::string error;

  bool IsOk() const { return value.has_value(); }

  // Or use "{.value = v}"
  static Result<T> Ok(T &&v) {
    return {.value = std::move(v)};
  }

  // Or use "{.error = s}"
  static Result<T> Err(const std::string &s) {
    return {.error = s};
  }
};

using OkOrErr = Result<bool>;
static const OkOrErr kOK = {.value = true};

} /* namespace protobag */
