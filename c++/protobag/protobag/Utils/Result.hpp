#pragma once

#include <optional>

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
