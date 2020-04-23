#include "gtest/gtest.h"

#include <filesystem>

#include "protobag/Utils/Tempfile.hpp"


using namespace protobag;

namespace fs = std::filesystem;


TEST(TempfileTest, TestTempfile) {
  auto res = CreateTempfile();
  EXPECT_TRUE(res.IsOk());
  EXPECT_TRUE(fs::is_regular_file(*res.value));
}

TEST(TempfileTest, TestTempdir) {
  auto res = CreateTempdir();
  EXPECT_TRUE(res.IsOk());
  EXPECT_TRUE(fs::is_directory(*res.value));
}
