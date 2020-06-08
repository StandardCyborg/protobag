#pragma once

#ifndef PROTOBAG_TEST_DEFAULT_FIXTURES_DIR
#define PROTOBAG_TEST_DEFAULT_FIXTURES_DIR "/opt/protobag/protobag_test/fixtures"
#endif
// #define PROTOBAG_TEST_DEFAULT_FIXTURES_DIR @PROTOBAG_TEST_DEFAULT_FIXTURES_DIR@

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <filesystem>

#include "protobag/archive/Archive.hpp"
#include "protobag/archive/MemoryArchive.hpp"
#include "protobag/ReadSession.hpp"
#include "protobag/WriteSession.hpp"

namespace protobag_test {

namespace fs = std::filesystem;

template <typename PathT>
inline fs::path GetFixture(PathT fname) {
  auto p = std::getenv("PROTOBAG_TEST_FIXTURES_DIR");
  if (p) {
    return fs::path{p} / fname;
  } else {
    return fs::path{PROTOBAG_TEST_DEFAULT_FIXTURES_DIR} / fname;
  }
}

inline fs::path CreateTempDir(const std::string &testname, bool clean=true) {
  fs::path tempdir = 
    fs::temp_directory_path() / 
        fs::path("protobag-test") /
        fs::path(testname);
  fs::create_directories(tempdir);
  if (clean) {
    fs::remove_all(tempdir);
    fs::create_directories(tempdir);
  }
  return tempdir;
}


inline
protobag::archive::Archive::Ptr OpenAndCheck(
    protobag::archive::Archive::Spec spec) {
  
  auto result = protobag::archive::Archive::Open(spec);
  if (!result.IsOk()) {
    throw std::runtime_error(result.error);
  }

  auto ar = *result.value;
  if (!ar) {
    throw std::runtime_error("Null pointer exception: bad result object");
  }

  return ar;
}

template <typename EntryContainerT>
std::shared_ptr<protobag::archive::MemoryArchive> CreateMemoryArchive(
    const EntryContainerT &entries) {

  auto buffer = protobag::archive::MemoryArchive::Create();
  
  {
    auto maybe_ws = protobag::WriteSession::Create({
      .archive_spec = {
        .mode="write",
        .format="memory",
        .memory_archive=buffer,
      },
    });
    if (!maybe_ws.IsOk()) {
      throw std::runtime_error(maybe_ws.error);
    }
    auto &writer = **maybe_ws.value;

    for (const auto &entry : entries) {
      auto status = writer.WriteEntry(entry);
      if (!status.IsOk()) {
        throw std::runtime_error(status.error);
      }
    }
    writer.Close();
  }

  return buffer;
}

template <typename EntryContainerT>
protobag::ReadSession::Ptr CreateInMemoryReadSession(
    const protobag::Selection &sel,
    const EntryContainerT &entries) {

  auto fixture = CreateMemoryArchive(entries);
  
  auto result = protobag::ReadSession::Create({
    .archive_spec = {
      .mode="read",
      .format="memory",
      .memory_archive=fixture,
    },
    .selection = sel,
  });
  if (!result.IsOk()) {
    throw std::runtime_error(result.error);
  }

  auto rs = *result.value;
  if (!rs) {
    throw std::runtime_error("Null pointer exception: bad result object");
  }

  return rs;
}


// GTest appears to lack a sequence checker... so here is one!
template <typename ExpectedBT,
          typename ExpectedET,
          typename ActualBT,
          typename ActualET>
::testing::AssertionResult AssertRangesEqual(
    const ExpectedBT &expected_begin,
    const ExpectedET &expected_end,
    const ActualBT &actual_begin,
    const ActualET &actual_end,
    const char *expected_name,
    const char *actual_name) {
  const size_t size_expected = expected_end - expected_begin;
  const size_t size_actual = actual_end - actual_begin;
  if (size_expected != size_actual) {
    return
        ::testing::AssertionFailure() <<
        "Size mistmatch: " <<
        expected_name << " (size " << size_expected << ") != " <<
        actual_name << " (size " << size_actual << ")";
  }

  const size_t kMaxMismatchesToPrint = 5;
  size_t num_mismatches = 0;
  auto failure = ::testing::AssertionFailure();
  failure <<
      "Sequences ( " << expected_name << " vs " << actual_name <<
      " ) mismatch: ";
  for (size_t i = 0; i < size_expected; ++i) {
    const auto& expected_val = *(expected_begin + i);
    const auto& actual_val = *(actual_begin + i);
    if (expected_val != actual_val) {
      ++num_mismatches;
      if (num_mismatches < kMaxMismatchesToPrint) {
        failure << "i=" << i << " (expected: " << expected_val << " vs " <<
            "actual: " << actual_val << ") ";
      }
    }
  }

  if (num_mismatches == 0) {
    return ::testing::AssertionSuccess();
  }

  if (num_mismatches >= kMaxMismatchesToPrint) {
    failure << "... ";
  }
  failure << "(total: " << num_mismatches << ")";
  return failure;
}

template <typename ExpectedT, typename ActualT>
::testing::AssertionResult AssertSequencesEqual(
    const ExpectedT &expected,
    const ActualT &actual,
    const char *expected_name,
    const char *actual_name) {

  return AssertRangesEqual(
      expected.begin(),
      expected.end(),
      actual.begin(),
      actual.end(),
      expected_name,
      actual_name);
}



} /* namespace protobag_test */

#define EXPECT_SEQUENCES_EQUAL(expected, actual) \
  EXPECT_TRUE( \
    ::protobag_test::AssertSequencesEqual( \
      expected, \
      actual, \
      #expected, \
      #actual)); \

#define EXPECT_RANGES_EQUAL(expected_begin, expected_end, actual_begin, actual_end) \
  EXPECT_TRUE( \
    ::protobag_test::AssertRangesEqual( \
      expected_begin, \
      expected_end, \
      actual_begin, \
      actual_end, \
      #expected_begin, \
      #actual_begin)); \

#define EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual) do { \
    auto e2 = expected; \
    auto a2 = actual; \
    std::sort(e2.begin(), e2.end()); \
    std::sort(a2.begin(), a2.end()); \
    EXPECT_TRUE( \
      ::protobag_test::AssertSequencesEqual( \
        e2, \
        a2, \
        #expected, \
        #actual)); \
  } while(0)