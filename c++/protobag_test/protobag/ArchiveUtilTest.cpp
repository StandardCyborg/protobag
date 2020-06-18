#include "gtest/gtest.h"

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


// ============================================================================
// GetAllFilesRecursive

TEST(ArchiveUtilTest, GetAllFilesRecursiveNotExists) {
  static const std::string kPathNoExists = "/path-no-exists";
  ASSERT_FALSE(fs::exists(kPathNoExists));

  auto maybeFiles = GetAllFilesRecursive(kPathNoExists); 
  EXPECT_FALSE(maybeFiles.IsOk()) << maybeFiles.error;
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
