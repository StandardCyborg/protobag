#include "gtest/gtest.h"

#include <iostream>

#include "protobag/archive/Archive.hpp"

using namespace protobag;
using namespace protobag::archive;

TEST(ArchiveTest, TestBase) {
  auto ar = Archive::Open();

  std::cout << "archive" << std::endl;
}
