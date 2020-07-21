#include "gtest/gtest.h"

#include <iostream>

#include "protobag/archive/Archive.hpp"

using namespace protobag;
using namespace protobag::archive;

TEST(ArchiveTest, TestBase) {
  auto maybeAr = Archive::Open();
  ASSERT_TRUE(maybeAr.IsOk()) << maybeAr.error;
}
