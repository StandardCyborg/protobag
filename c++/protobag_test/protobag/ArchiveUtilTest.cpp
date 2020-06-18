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

void CheckThatCanReadAndIsEmpty(const std::string &path) {

}

TEST(ArchiveUtilTest, CreateArchiveAtPathEmptyTar) {
  auto tempdir = CreateTestTempdir("ArchiveUtilTest.CreateArchiveAtPathEmptyTar");
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


// ============================================================================
// CreateArchiveAtPathFromFiles
