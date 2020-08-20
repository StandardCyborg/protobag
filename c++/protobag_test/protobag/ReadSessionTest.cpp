#include "gtest/gtest.h"

#include <algorithm>
#include <exception>
#include <vector>
#include <unordered_map>

#include "protobag/archive/Archive.hpp"
#include "protobag/BagIndexBuilder.hpp"
#include "protobag/Entry.hpp"
#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/StdMsgUtils.hpp"
#include "protobag/ReadSession.hpp"

#include "protobag_test/Utils.hpp"


using namespace protobag;
using namespace protobag::archive;
using namespace protobag_test;

inline 
ReadSession::Ptr OpenReaderAndCheck(const ReadSession::Spec &spec) {
  auto result = ReadSession::Create(spec);
  if (!result.IsOk()) {
    throw std::runtime_error(result.error);
  }

  auto r = *result.value;
  if (!r) {
    throw std::runtime_error("Null pointer exception: bad result object");
  }

  return r;
}

inline
Entry CreateStampedWithEntryname(
    const std::string &entryname,
    Entry entry) {
  
  entry.entryname = entryname;
  return entry;
}

// So that we can make the tests in this module independent of WriteSession,
// we manually create protobag fixtures using the utility below, which
// simulates what a WriteSession + DirectoryArchive would leave on disk.
template <typename EntryContainerT>
void WriteEntriesAndIndex(
    const std::string &path,
    const EntryContainerT &entries,
    const std::string &format="directory") {

  auto maybe_dar = Archive::Open({
    .mode="write",
    .path=path,
    .format=format,
  });
  if (!maybe_dar.IsOk()) {
    throw std::runtime_error(maybe_dar.error);
  }
  auto dar = *maybe_dar.value;

  // Write entries
  BagIndexBuilder::UPtr builder(new BagIndexBuilder());
  for (const auto &entry : entries) {
    auto maybe_m_bytes = PBFactory::ToBinaryString(entry.msg);
    if (!maybe_m_bytes.IsOk()) {
      throw std::runtime_error(maybe_m_bytes.error);
    }

    auto status = dar->Write(entry.entryname, *maybe_m_bytes.value);
    if (!status.IsOk()) {
      throw std::runtime_error(status.error);
    }

    builder->Observe(entry, entry.entryname);
  }

  // Write index
  BagIndex index = BagIndexBuilder::Complete(std::move(builder));
  {
    auto index_entry = CreateStampedWithEntryname(
      "/_protobag_index/bag_index/1337.1337.stampedmsg.protobin",
      Entry::CreateStamped(
      "/_protobag_index/bag_index", 1337, 1337, index));

    auto maybe_m_bytes = PBFactory::ToBinaryString(index_entry.msg);
    if (!maybe_m_bytes.IsOk()) {
      throw std::runtime_error(maybe_m_bytes.error);
    }

    auto status = dar->Write(index_entry.entryname, *maybe_m_bytes.value);
    if (!status.IsOk()) {
      throw std::runtime_error(status.error);
    }

  }

  dar->Close();
}

template <typename EntryContainerT>
void ReadAllEntriesAndCheck(
    const std::string &path,
    const EntryContainerT &expected_entries) {

  auto rp = OpenReaderAndCheck(ReadSession::Spec::ReadAllFromPath(path));
  auto &reader = *rp;

  std::vector<Entry> actual_entries;
  bool still_reading = true;
  bool has_index = false;
  do {
    MaybeEntry maybe_next = reader.GetNext();
    if (maybe_next.IsEndOfSequence()) {
      still_reading = false;
      break;
    }
    ASSERT_TRUE(maybe_next.IsOk()) << maybe_next.error;
    const auto &entry = *maybe_next.value;
    if (entry.entryname.find("/_protobag_index") != std::string::npos) {
      has_index = true;
    } else {
      actual_entries.push_back(*maybe_next.value);
    }
  } while(still_reading);


  std::vector<std::string> expected_names;
  std::unordered_map<std::string, Entry> name_to_expected;
  {
    for (const auto &eentry : expected_entries) {
      name_to_expected[eentry.entryname] = eentry;
      expected_names.push_back(eentry.entryname);
    }
  }
  
  std::vector<std::string> actual_names;
  std::unordered_map<std::string, Entry> name_to_actual;
  {
    for (const auto &aentry : actual_entries) {
      name_to_actual[aentry.entryname] = aentry;
      actual_names.push_back(aentry.entryname);
    }
  }

  // Check just the entry name lists match
  EXPECT_SORTED_SEQUENCES_EQUAL(expected_names, actual_names);
  ASSERT_EQ(expected_names.size(), actual_names.size());

  // Check contents
  for (const auto &expected_me : name_to_expected) {
    const auto &actual = name_to_actual[expected_me.first];
    auto expected = expected_me.second;
    
    if (expected.IsStampedMessage()) {

      auto e_tt = expected.GetTopicTime();
      ASSERT_TRUE(e_tt.has_value());
      auto a_tt = actual.GetTopicTime();
      ASSERT_TRUE(a_tt.has_value());
      EXPECT_EQ(e_tt->topic(), a_tt->topic());
      EXPECT_EQ(e_tt->timestamp().seconds(), a_tt->timestamp().seconds());
      EXPECT_EQ(e_tt->timestamp().nanos(), a_tt->timestamp().nanos());


      // Unpack expected, actual is already unpacked
      auto maybe_ee = expected.UnpackFromStamped();
      ASSERT_TRUE(maybe_ee.IsOk());
      expected = *maybe_ee.value;
    } else {
      EXPECT_EQ(
        PBToString(actual.msg),
        PBToString(expected.msg));
    }
      
    EXPECT_TRUE(actual.EntryDataEqualTo(expected)) << 
      "Actual: " << actual.ToString() << 
      "\nExpected:\n" << expected.ToString();
  }
}

TEST(ReadSessionTest, DirectoryTestMessages) {
  auto testdir = CreateTestTempdir("ReadSessionTest.DirectoryTestMessages");

  static const std::vector<Entry> kExpectedEntries = {
    Entry::Create("/moof", ToStringMsg("moof")),
    Entry::Create("/hi_1337", ToIntMsg(1337)),
  };

  WriteEntriesAndIndex(testdir, kExpectedEntries);

  ReadAllEntriesAndCheck(testdir, kExpectedEntries);
}

TEST(ReadSessionTest, DirectoryTestStampedMessages) {
  auto testdir = CreateTestTempdir("ReadSessionTest.DirectoryTestStampedMessages");

  static const std::vector<Entry> kExpectedEntries = {
    CreateStampedWithEntryname(
      "/topic1/0.0.stampedmsg.protobin",
      Entry::CreateStamped("/topic1", 0, 0, ToStringMsg("foo"))),
    CreateStampedWithEntryname(
      "/topic2/0.0.stampedmsg.protobin",
      Entry::CreateStamped("/topic2", 0, 0, ToIntMsg(1337))),
    CreateStampedWithEntryname(
      "/topic1/1.0.stampedmsg.protobin",
      Entry::CreateStamped("/topic1", 1, 0, ToStringMsg("bar"))),
  };

  WriteEntriesAndIndex(testdir, kExpectedEntries);

  ReadAllEntriesAndCheck(testdir, kExpectedEntries);
}

TEST(ReadSessionTest, DirectoryTestRawMessages) {
  auto testdir = CreateTestTempdir("ReadSessionTest.DirectoryTestRawMessages");

  static const std::vector<Entry> kExpectedEntries = {
    Entry::CreateRawFromBytes("/i_am_raw", "i am raw data"),
    Entry::CreateRawFromBytes("/i_am_raw2", "i am also raw data"),
  };

  WriteEntriesAndIndex(testdir, kExpectedEntries);

  ReadAllEntriesAndCheck(testdir, kExpectedEntries);
}
