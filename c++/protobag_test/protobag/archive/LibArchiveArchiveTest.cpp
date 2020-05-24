#include "gtest/gtest.h"

#include <fstream>

#include "protobag_test/Utils.hpp"

#include "protobag/archive/Archive.hpp"

using namespace protobag;
using namespace protobag::archive;
using namespace protobag_test;

TEST(LibArchiveArchiveTest, ReadDoesNotExist) {
  auto tempdir = CreateTempDir("LibArchiveArchive.ReadDoesNotExist");
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
  auto testdir = CreateTempDir("LibArchiveArchiveTest.TestWrite");
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

  std::cout << "TestRead" << std::endl;
  for (auto &a : actual) {
    std::cout << a << std::endl;
  }
  std::cout << "TestRead" << std::endl;

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
