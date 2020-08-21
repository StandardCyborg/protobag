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

#include "gtest/gtest.h"

#include <fstream>

#include "protobag_test/Utils.hpp"

#include "protobag/archive/Archive.hpp"

using namespace protobag;
using namespace protobag::archive;
using namespace protobag_test;

TEST(LibArchiveArchiveTest, ReadDoesNotExist) {
  auto tempdir = CreateTestTempdir("LibArchiveArchive.ReadDoesNotExist");
  fs::remove_all(tempdir);
  auto result = Archive::Open({
    .mode="read",
    .path=tempdir.string(),
    .format="zip",
  });
  EXPECT_FALSE(result.IsOk());
  EXPECT_FALSE(result.error.empty());
}


TEST(LibArchiveArchiveTest, TestWrite) {
  auto testdir = CreateTestTempdir("LibArchiveArchiveTest.TestWrite");
  auto test_file = testdir / "test.tar";
  {
    auto ar = OpenAndCheck({
      .mode="write",
      .path=test_file,
      .format="tar",
    });

    Result<bool> res;
    res = ar->Write("foo", "foo");
    EXPECT_TRUE(res.IsOk()) << res.error;
    res = ar->Write("bar/bar", "bar");
    EXPECT_TRUE(res.IsOk()) << res.error;
  }

  EXPECT_TRUE(fs::is_regular_file(test_file));
}


TEST(LibArchiveArchiveTest, TestRead) {
  auto ar = OpenAndCheck({
    .mode="read",
    .path=GetFixture("test.tar"),
    .format="tar",
  });

  auto actual = ar->GetNamelist();
  std::vector<std::string> expected = {"foo", "bar/bar"};

  EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);

  {
    auto res = ar->ReadAsStr("does-not-exist");
    EXPECT_FALSE(res.IsOk());
    EXPECT_FALSE(res.error.empty()) << res.error;
    EXPECT_EQ(res, Archive::ReadStatus::EntryNotFound());
    EXPECT_TRUE(res.IsEntryNotFound());
  }
  {
    auto res = ar->ReadAsStr("foo");
    EXPECT_TRUE(res.IsOk()) << res.error;
    auto value = *res.value;
    EXPECT_EQ(value, "foo");
  }
  {
    auto res = ar->ReadAsStr("bar/bar");
    EXPECT_TRUE(res.IsOk()) << res.error;
    auto value = *res.value;
    EXPECT_EQ(value, "bar");
  }
}

// TODO: test zip
