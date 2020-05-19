#include "gtest/gtest.h"

#include <algorithm>
#include <exception>
#include <vector>

#include "protobag/Entry.hpp"
#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/StdMsgUtils.hpp"
#include "protobag/ReadSession.hpp"

#include "protobag_test/Utils.hpp"


using namespace protobag;
using namespace protobag_test;

static const std::vector<Entry> kExpectedEntries = {
  Entry::Create("/topic1", 0, ToStringMsg("foo")),
  Entry::Create("/topic2", 0, ToIntMsg(1337)),
  Entry::Create("/topic1", 1, ToStringMsg("bar")),
};

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


TEST(ReadSessionDirectory, TestBasic) {
  auto fixture_path = GetFixture("ReadSessionDirectory.TestBasic");

  auto rp = OpenReaderAndCheck(
    ReadSession::Spec::ReadAllFromPath(fixture_path));

  auto &reader = *rp;

  
  std::vector<Entry> actual_entries;
  bool still_reading = true;
  do {
    MaybeEntry maybe_next = reader.GetNext();
    if (maybe_next.IsEndOfSequence()) {
      still_reading = false;
      break;
    }

    ASSERT_TRUE(maybe_next.IsOk()) << maybe_next.error;
    
    actual_entries.push_back(*maybe_next.value);

  } while(still_reading);

  auto expected_entries = kExpectedEntries;
  // std::sort(expected_entries.begin(), expected_entries.end());
  // std::sort(actual_entries.begin(), actual_entries.end());

  ASSERT_EQ(actual_entries.size(), expected_entries.size());
  for (size_t i = 0; i < actual_entries.size(); ++i) {
    auto expected = expected_entries[i];
    auto actual = actual_entries[i];
    EXPECT_EQ(actual.topic, expected.topic);
    
    auto ToTextFormatOrThrow = [] (const auto &entry) -> auto {
      auto maybe_txt = PBFactory::ToTextFormatString(entry);
      if (!maybe_txt.IsOk()) {
        throw std::runtime_error(maybe_txt.error);
      }
      return *maybe_txt.value;
    };
    EXPECT_EQ(
      ToTextFormatOrThrow(actual.stamped_msg),
      ToTextFormatOrThrow(expected.stamped_msg));
  }
}
