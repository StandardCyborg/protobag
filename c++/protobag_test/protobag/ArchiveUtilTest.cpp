#include "gtest/gtest.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "protobag/ArchiveUtil.hpp"
#include "protobag_test/Utils.hpp"

namespace fs = std::filesystem;

using namespace protobag;
using namespace protobag_test;


void FullWithFoo12Fixture(const fs::path &basedir) {
  fs::create_directories(basedir);
  fs::create_directories(basedir / "foo");
  fs::create_directories(basedir / "empty_dir");
  { std::ofstream f(basedir / "foo" / "f1"); f << "f1"; }
  { std::ofstream f(basedir / "foo" / "f2"); f << "f2"; }
  { std::ofstream f(basedir / "bar"); f << "bar"; }
}

void CheckArchiveHasFoo12Data(const std::string &path) {
  auto ar = OpenAndCheck({
      .mode="read",
      .path=path,
    });
  
  auto actual = ar->GetNamelist();
  std::vector<std::string> expected = {
    "bar",
    "foo/f1",
    "foo/f2",
  };
  EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);

  {
    auto res = ar->ReadAsStr("bar");
    ASSERT_TRUE(res.IsOk()) << res.error;
    auto value = *res.value;
    EXPECT_EQ(value, "bar");
  }
  {
    auto res = ar->ReadAsStr("foo/f1");
    ASSERT_TRUE(res.IsOk()) << res.error;
    auto value = *res.value;
    EXPECT_EQ(value, "f1");
  }
  {
    auto res = ar->ReadAsStr("foo/f2");
    ASSERT_TRUE(res.IsOk()) << res.error;
    auto value = *res.value;
    EXPECT_EQ(value, "f2");
  }
}


// ============================================================================
// ReadFile

TEST(ArchiveUtilTest, ReadFileNotExists) {
  static const std::string kPathNoExists = "/path-no-exists";
  ASSERT_FALSE(fs::exists(kPathNoExists));

  auto result = ReadFile(kPathNoExists);
  EXPECT_EQ(result, "");
}

TEST(ArchiveUtilTest, ReadFileSomeData) {
  auto tempdir =
    CreateTestTempdir("ArchiveUtilTest.IsDirectoryNotADirectory");
  fs::path plain_file = tempdir / "foo.txt";
  { 
    std::ofstream f(plain_file);
    f << "foo";
  }

  auto result = ReadFile(plain_file);
  EXPECT_EQ(result, "foo");
}



// ============================================================================
// IsDirectory

TEST(ArchiveUtilTest, IsDirectoryNotExists) {
  static const std::string kPathNoExists = "/path-no-exists";
  ASSERT_FALSE(fs::exists(kPathNoExists));

  auto status = IsDirectory(kPathNoExists); 
  EXPECT_TRUE(status.IsOk()) << status.error;
  EXPECT_FALSE(*status.value);
}

TEST(ArchiveUtilTest, IsDirectoryNotADirectory) {
  auto tempdir =
    CreateTestTempdir("ArchiveUtilTest.IsDirectoryNotADirectory");
  fs::path plain_file = tempdir / "foo.txt";
  { std::ofstream f(plain_file); }
  fs::path symlink = tempdir / "foo-link";
  { 
    auto ret = std::system(
      fmt::format(
        "ln -s {} {}", plain_file.string(), symlink.string()).c_str());
    ASSERT_EQ(ret, 0);
  }

  {
    auto status = IsDirectory(plain_file); 
    EXPECT_TRUE(status.IsOk()) << status.error;
    EXPECT_FALSE(*status.value);
  }

  {
    auto status = IsDirectory(symlink); 
    EXPECT_TRUE(status.IsOk()) << status.error;
    EXPECT_FALSE(*status.value);
  }
}

TEST(ArchiveUtilTest, IsDirectoryIsADirectory) {
  auto tempdir =
    CreateTestTempdir("ArchiveUtilTest.IsDirectoryIsADirectory");
  
  auto status = IsDirectory(tempdir); 
  EXPECT_TRUE(status.IsOk()) << status.error;
  EXPECT_TRUE(*status.value);
}



// ============================================================================
// GetAllFilesRecursive

TEST(ArchiveUtilTest, GetAllFilesRecursiveNotExists) {
  static const std::string kPathNoExists = "/path-no-exists";
  ASSERT_FALSE(fs::exists(kPathNoExists));

  auto maybeFiles = GetAllFilesRecursive(kPathNoExists); 
  EXPECT_FALSE(maybeFiles.IsOk()) << maybeFiles.error;
}

TEST(ArchiveUtilTest, GetAllFilesRecursiveEmpty) {
  auto tempdir =
    CreateTestTempdir("ArchiveUtilTest.GetAllFilesRecursiveEmpty");

  auto maybeFiles = GetAllFilesRecursive(tempdir); 
  EXPECT_TRUE(maybeFiles.IsOk()) << maybeFiles.error;
  EXPECT_TRUE(maybeFiles.value->empty());
}

TEST(ArchiveUtilTest, GetAllFilesRecursiveSomeFiles) {
  auto testdir =
    CreateTestTempdir("ArchiveUtilTest.GetAllFilesRecursiveEmpty");
  FullWithFoo12Fixture(testdir);

  auto maybeFiles = GetAllFilesRecursive(testdir); 
  EXPECT_TRUE(maybeFiles.IsOk()) << maybeFiles.error;
  auto files = *maybeFiles.value;
  std::sort(files.begin(), files.end());

  decltype(files) expected = {
    testdir / "bar",
    testdir / "foo/f1",
    testdir / "foo/f2",
  };

  EXPECT_EQ(files, expected);
}



// ============================================================================
// UnpackArchiveToDir

TEST(ArchiveUtilTest, UnpackArchiveToDirNotExists) {
  static const std::string kPathNoExists = "/path-no-exists";
  ASSERT_FALSE(fs::exists(kPathNoExists));

  auto status = UnpackArchiveToDir(kPathNoExists, kPathNoExists); 
  EXPECT_FALSE(status.IsOk()) << status.error;
}

TEST(ArchiveUtilTest, UnpackArchiveToDirUnpackTar) {
  auto testdir =
    CreateTestTempdir("ArchiveUtilTest.UnpackArchiveToDirUnpackTar");

  auto status = UnpackArchiveToDir(GetFixture("test.tar"), testdir); 
  EXPECT_TRUE(status.IsOk()) << status.error;

  EXPECT_EQ(ReadFile(testdir / "foo"), "foo");
  EXPECT_EQ(ReadFile(testdir / "bar/bar"), "bar");
  CheckHasCommand("ls");
  std::string cmd = fmt::format(
    "ls {} | wc -l",
    testdir.u8string());
  RunCMDAndCheckOutput(cmd, "2");
}

TEST(ArchiveUtilTest, UnpackArchiveToDirUnpackZip) {
  auto testdir =
    CreateTestTempdir("ArchiveUtilTest.UnpackArchiveToDirUnpackZip");

  auto status = UnpackArchiveToDir(GetFixture("test.zip"), testdir); 
  EXPECT_TRUE(status.IsOk()) << status.error;

  EXPECT_EQ(ReadFile(testdir / "foo"), "foo");
  EXPECT_EQ(ReadFile(testdir / "bar/bar"), "bar");
  CheckHasCommand("ls");
  std::string cmd = fmt::format(
    "ls {} | wc -l",
    testdir.u8string());
  RunCMDAndCheckOutput(cmd, "2");
}


// ============================================================================
// CreateArchiveAtPath

void CheckArchiveHasFooAndOnlyFoo(const std::string &path) {
  auto ar = OpenAndCheck({
      .mode="read",
      .path=path,
    });
  
  auto actual = ar->GetNamelist();
  for (auto &a : actual) {
    a = fs::path(a).filename();
  }
  std::vector<std::string> expected = {"foo"};

  EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);

  {
    auto foo_entryname = ar->GetNamelist()[0];
    auto res = ar->ReadAsStr(foo_entryname);
    ASSERT_TRUE(res.IsOk()) << res.error << " " << foo_entryname;
    auto value = *res.value;
    EXPECT_EQ(value, "foo");
  }
}


TEST(ArchiveUtilTest, CreateArchiveAtPathEmptyTar) {
  auto tempdir =
    CreateTestTempdir("ArchiveUtilTest.CreateArchiveAtPathEmptyTar");
  auto outpath = tempdir / "test.tar";
  auto status = CreateArchiveAtPath({}, outpath.u8string());
  ASSERT_TRUE(status.IsOk()) << status.error;
  ASSERT_TRUE(fs::exists(outpath)) << outpath;

  CheckHasCommand("tar");
  std::string cmd = fmt::format(
    "tar --list --verbose --file={} | wc -l",
    outpath.u8string());
  RunCMDAndCheckOutput(cmd, "0");
}

TEST(ArchiveUtilTest, CreateArchiveAtPathHasDataTar) {
  auto testdir =
    CreateTestTempdir("ArchiveUtilTest.CreateArchiveAtPathHasDataTar");
  auto archive_input = testdir / "foo";
  { std::ofstream f(archive_input); f << "foo"; }
  ASSERT_EQ(ReadFile(archive_input), "foo");
  
  auto outpath = testdir / "output.tar";
  auto status = CreateArchiveAtPath(
    {archive_input},
    outpath.u8string());
  ASSERT_TRUE(status.IsOk()) << status.error;
  ASSERT_TRUE(fs::exists(outpath)) << outpath;

  CheckArchiveHasFooAndOnlyFoo(outpath);

  // Make sure tar can read
  {
    CheckHasCommand("tar");
    std::string cmd = fmt::format(
      "tar --list --verbose --file={} | grep foo | wc -l",
      outpath.u8string());
    RunCMDAndCheckOutput(cmd, "1");
  }
}

TEST(ArchiveUtilTest, CreateArchiveAtPathEmptyZip) {
  auto tempdir =
    CreateTestTempdir("ArchiveUtilTest.CreateArchiveAtPathEmptyZip");
  auto outpath = tempdir / "test.zip";
  auto status = CreateArchiveAtPath({}, outpath.u8string());
  ASSERT_TRUE(status.IsOk()) << status.error;
  ASSERT_TRUE(fs::exists(outpath)) << outpath;

  CheckHasCommand("unzip");
  std::string cmd = 
    fmt::format("unzip -l {} 2>&1 | grep \"zipfile is empty\" | wc -l",
    outpath.u8string());
      // NB: unzip exits non-zero for an empty zip file
  RunCMDAndCheckOutput(cmd, "1");
}

TEST(ArchiveUtilTest, CreateArchiveAtPathHasDataZip) {
  auto testdir =
    CreateTestTempdir("ArchiveUtilTest.CreateArchiveAtPathHasDataZip");
  auto archive_input = testdir / "foo";
  { std::ofstream f(archive_input); f << "foo"; }
  ASSERT_EQ(ReadFile(archive_input), "foo");
  
  auto outpath = testdir / "output.zip";
  auto status = CreateArchiveAtPath(
    {archive_input},
    outpath.u8string());
  ASSERT_TRUE(status.IsOk()) << status.error;
  ASSERT_TRUE(fs::exists(outpath)) << outpath;

  CheckArchiveHasFooAndOnlyFoo(outpath);

  // Make sure unzip can read
  {
    CheckHasCommand("unzip");
    std::string cmd = fmt::format(
      "unzip -l {} | grep foo | wc -l",
      outpath.u8string());
    RunCMDAndCheckOutput(cmd, "1");
  }
}



// ============================================================================
// CreateArchiveAtPathFromFiles

TEST(ArchiveUtilTest, CreateArchiveAtPathFromFilesTar) {
  auto testdir =
    CreateTestTempdir("ArchiveUtilTest.CreateArchiveAtPathHasDataTar");
  auto archive_input = testdir / "archive_input";
  FullWithFoo12Fixture(archive_input);

  auto outpath = testdir / "output.tar";
  auto status = CreateArchiveAtPathFromDir(archive_input, outpath.u8string());
  ASSERT_TRUE(status.IsOk()) << status.error;
  ASSERT_TRUE(fs::exists(outpath)) << outpath;

  CheckArchiveHasFoo12Data(outpath);

  // Make sure tar can read
  {
    CheckHasCommand("tar");
    std::string cmd = fmt::format(
      "tar --list --verbose --file={} | wc -l",
      outpath.u8string());
    RunCMDAndCheckOutput(cmd, "3");
  }
}

TEST(ArchiveUtilTest, CreateArchiveAtPathFromFilesZip) {
  auto testdir =
    CreateTestTempdir("ArchiveUtilTest.CreateArchiveAtPathHasDataZip");
  auto archive_input = testdir / "archive_input";
  FullWithFoo12Fixture(archive_input);

  auto outpath = testdir / "output.zip";
  auto status = CreateArchiveAtPathFromDir(archive_input, outpath.u8string());
  ASSERT_TRUE(status.IsOk()) << status.error;
  ASSERT_TRUE(fs::exists(outpath)) << outpath;

  CheckArchiveHasFoo12Data(outpath);

  // Make sure unzip can read
  {
    CheckHasCommand("unzip");
    {
      std::string cmd = fmt::format(
        "unzip -l {} | grep foo | wc -l",
        outpath.u8string());
      RunCMDAndCheckOutput(cmd, "2");
    }
    {
      std::string cmd = fmt::format(
        "unzip -l {} | grep bar | wc -l",
        outpath.u8string());
      RunCMDAndCheckOutput(cmd, "1");
    }
  }
}
