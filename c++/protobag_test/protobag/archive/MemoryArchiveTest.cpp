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
#include "protobag/archive/MemoryArchive.hpp"

using namespace protobag;
using namespace protobag::archive;
using namespace protobag_test;


TEST(MemoryArchiveTest, ReadDoesNotExist) {
  auto result = Archive::Open({
    .mode="read",
    .format="memory",
  });
  EXPECT_TRUE(result.IsOk());
}


TEST(MemoryArchiveTest, ReadEmpty) {
  auto ar = OpenAndCheck({
    .mode="read",
    .format="memory",
  });
  
  auto names = ar->GetNamelist();
  EXPECT_TRUE(names.empty());

  auto res = ar->ReadAsStr("does/not/exist");
  EXPECT_EQ(res, Archive::ReadStatus::EntryNotFound());
  EXPECT_TRUE(res.IsEntryNotFound());
}


TEST(MemoryArchiveTest, TestNamelist) {
  auto fixture = MemoryArchive::Create(
    {
      {"/foo/f1", ""},
      {"/foo/f2", ""},
    }
  );

  auto ar = OpenAndCheck({
    .mode="read",
    .format="memory",
    .memory_archive=fixture,
  });
  
  auto actual = ar->GetNamelist();
  std::vector<std::string> expected = {"/foo/f1", "/foo/f2"};
  EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);
}


TEST(MemoryArchiveTest, TestRead) {
  auto fixture = MemoryArchive::Create(
    {
      {"/foo/f1", "bar"},
    }
  );
  auto ar = OpenAndCheck({
    .mode="read",
    .format="memory",
    .memory_archive=fixture,
  });
  
  auto actual = ar->GetNamelist();
  std::vector<std::string> expected = {"/foo/f1"};
  EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);

  {
    auto res = ar->ReadAsStr("does-not-exist");
    EXPECT_FALSE(res.IsOk());
    EXPECT_FALSE(res.error.empty()) << res.error;
    EXPECT_EQ(res, Archive::ReadStatus::EntryNotFound());
    EXPECT_TRUE(res.IsEntryNotFound());
  }

  {
    auto res = ar->ReadAsStr("/foo/f1");
    EXPECT_TRUE(res.IsOk()) << res.error;
    auto value = *res.value;
    EXPECT_EQ(value, "bar");
  }

  // The leading `/` is optional
  {
    auto res = ar->ReadAsStr("foo/f1");
    EXPECT_TRUE(res.IsOk());
    auto value = *res.value;
    EXPECT_EQ(value, "bar");
  }
}


TEST(MemoryArchiveTest, TestWriteAndRead) {
  auto buffer = MemoryArchive::Create();

  {
    auto ar = OpenAndCheck({
      .mode="write",
      .format="memory",
      .memory_archive=buffer,
    });

    Result<bool> res;
    res = ar->Write("foo", "foo");
    EXPECT_TRUE(res.IsOk()) << res.error;
    res = ar->Write("bar/bar", "bar");
    EXPECT_TRUE(res.IsOk()) << res.error;
  }

  {
    auto actual_data = buffer->GetData();

    std::unordered_map<std::string, std::string> expected_data = {
      {"foo", "foo"},
      {"bar/bar", "bar"},
    };

    EXPECT_EQ(actual_data, expected_data);
  }

  // Now read using Archive interface
  {
    auto ar = OpenAndCheck({
      .mode="read",
      .format="memory",
      .memory_archive=buffer,
    });

    auto actual = ar->GetNamelist();
    std::vector<std::string> expected = {"/foo", "/bar/bar"};
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
      EXPECT_TRUE(res.IsOk());
      auto value = *res.value;
      EXPECT_EQ(value, "foo");
    }
    {
      auto res = ar->ReadAsStr("bar/bar");
      EXPECT_TRUE(res.IsOk());
      auto value = *res.value;
      EXPECT_EQ(value, "bar");
    }

    // The leading `/` is optional
    {
      auto res = ar->ReadAsStr("/bar/bar");
      EXPECT_TRUE(res.IsOk());
      auto value = *res.value;
      EXPECT_EQ(value, "bar");
    }
  }
}
