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
  fs::create_directories(testdir / "foo");
  fs::create_directories(testdir / "empty_dir");
  std::ofstream(testdir / "foo" / "f1");
  std::ofstream(testdir / "foo" / "f2");

  auto maybeFiles = GetAllFilesRecursive(testdir); 
  EXPECT_TRUE(maybeFiles.IsOk()) << maybeFiles.error;
  auto files = *maybeFiles.value;
  std::sort(files.begin(), files.end());

  decltype(files) expected = {
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


// ============================================================================
// CreateArchiveAtPath

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


// ============================================================================
// CreateArchiveAtPathFromFiles
