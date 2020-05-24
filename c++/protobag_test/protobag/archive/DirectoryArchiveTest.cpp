#include "gtest/gtest.h"

#include <fstream>

#include "protobag_test/Utils.hpp"

#include "protobag/archive/Archive.hpp"

using namespace protobag;
using namespace protobag::archive;
using namespace protobag_test;


TEST(DirectoryArchiveTest, ReadDoesNotExist) {
  auto tempdir = CreateTempDir("DirectoryArchiveTest.ReadDoesNotExist");
  fs::remove_all(tempdir);
  auto result = Archive::Open({
    .mode="read",
    .path=tempdir.string(),
    .format="directory",
  });
  EXPECT_FALSE(result.IsOk());
  EXPECT_FALSE(result.error.empty());
}


TEST(DirectoryArchiveTest, ReadEmpty) {
  auto ar = OpenAndCheck({
    .mode="read",
    .path=CreateTempDir("DirectoryArchiveTest.ReadEmpty").string(),
    .format="directory",
  });
  
  auto names = ar->GetNamelist();
  EXPECT_TRUE(names.empty());

  auto res = ar->ReadAsStr("does/not/exist");
  EXPECT_EQ(res, Archive::ReadStatus::EntryNotFound());
  EXPECT_TRUE(res.IsEntryNotFound());
}


TEST(DirectoryArchiveTest, TestNamelist) {
  auto testdir = CreateTempDir("DirectoryArchiveTest.TestNamelist");
  fs::create_directories(testdir / "foo");
  fs::create_directories(testdir / "empty_dir");
  std::ofstream(testdir / "foo" / "f1");
  std::ofstream(testdir / "foo" / "f2");

  auto ar = OpenAndCheck({
    .mode="read",
    .path=testdir,
    .format="directory",
  });

  auto actual = ar->GetNamelist();
  std::vector<std::string> expected = {"/foo/f1", "/foo/f2"};
  EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);
}


TEST(DirectoryArchiveTest, TestRead) {
  auto testdir = CreateTempDir("DirectoryArchiveTest.TestRead");
  fs::create_directories(testdir / "foo");
  std::ofstream f(testdir / "foo" / "f1");
  f << "bar";
  f.close();

  auto ar = OpenAndCheck({
    .mode="read",
    .path=testdir,
    .format="directory",
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
    EXPECT_TRUE(res.IsOk());
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


TEST(DirectoryArchiveTest, TestWriteAndRead) {
  auto testdir = CreateTempDir("DirectoryArchiveTest.TestWriteAndRead");

  {
    auto ar = OpenAndCheck({
      .mode="write",
      .path=testdir,
      .format="directory",
    });

    Result<bool> res;
    res = ar->Write("foo", "foo");
    EXPECT_TRUE(res.IsOk()) << res.error;
    res = ar->Write("bar/bar", "bar");
    EXPECT_TRUE(res.IsOk()) << res.error;
  }

  EXPECT_TRUE(fs::is_regular_file(testdir / "foo"));
  {
    std::ifstream f(testdir / "foo");
    std::string actual;
    f >> actual;
    EXPECT_EQ(actual, "foo");
  }
  
  EXPECT_TRUE(fs::is_regular_file(testdir / "bar" / "bar"));
  {
    std::ifstream f(testdir / "bar" / "bar");
    std::string actual;
    f >> actual;
    EXPECT_EQ(actual, "bar");
  }


  // Now read using Archive
  {
    auto ar = OpenAndCheck({
      .mode="read",
      .path=testdir,
      .format="directory",
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


TEST(DirectoryArchiveTest, TempfileSupport) {
  auto ar = OpenAndCheck({
    .mode="write",
    .path="<tempfile>",
    .format="directory",
  });

  EXPECT_NE(ar->GetSpec().path, "<tempfile>");
  EXPECT_TRUE(fs::is_directory(ar->GetSpec().path));
}