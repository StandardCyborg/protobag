#include "protobag/Utils/Tempfile.hpp"

#include <fmt/format.h>

#include <fstream>
#include <random>

namespace protobag {

namespace fs = std::filesystem;

// Create and return a random string of length `len`; we draw characters
// from a standard ASCII set
std::string CreateRandomString(size_t len) {
  static const char* alpha = 
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static const size_t n_alpha = strlen(alpha) - 1;
  
  thread_local static std::mt19937 rg{std::random_device{}()};
  thread_local static std::uniform_int_distribution<std::string::size_type> sample(0, n_alpha-1);

  std::string out("_", len);
  for (size_t i = 0; i < len; ++i) {
    out[i] = alpha[sample(rg)];
  }
  return out;
}

Result<fs::path> CreateTempfile(const std::string &suffix, size_t max_attempts) {
  for (size_t attempt = 0; attempt < max_attempts; ++attempt) {
    std::string fname = CreateRandomString(12) + suffix;
    fs::path p = fs::temp_directory_path() / fname;
    if (!fs::exists(p)) {
      std::ofstream f{p};  // Create the file
      if (!f.good()) {
        return {
          .error = fmt::format("Failed to create {}", p)
        };
      } else {
        return {.value = p};
      }
    }
  }
  return {.error = "Cannot create a tempfile"};
}

Result<fs::path> CreateTempdir(const std::string &suffix,size_t max_attempts) {
  for (size_t attempt = 0; attempt < max_attempts; ++attempt) {
    std::string dirname = CreateRandomString(12) + suffix;
    fs::path p = fs::temp_directory_path() / dirname;
    if (!fs::exists(p)) {
      std::error_code err;
      fs::create_directories(p, &err);
      if (err) {
        return {.error = 
          fmt::format("Error creating directory {}: {}", p, err.message)
        };
      } else {
        return {.value = p};
      }
    }
  }
  return {.error = "Cannot create a temp directory"};
}

} /* namespace protobag */
