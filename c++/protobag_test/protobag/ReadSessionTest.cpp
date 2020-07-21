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
  Entry::CreateStamped("/topic1", 0, 0, ToStringMsg("foo")),
  Entry::CreateStamped("/topic2", 0, 0, ToIntMsg(1337)),
  Entry::CreateStamped("/topic1", 1, 0, ToStringMsg("bar")),
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

// TODO need to massage out indexing and stuff ...
// TEST(ReadSessionMemory, TestBasic) {
// }

// FIXME: add re-index support or make index fixture
// TEST(ReadSessionDirectory, TestBasic) {
//   auto fixture_path = GetFixture("ReadSessionDirectory.TestBasic");

//   auto rp = OpenReaderAndCheck(
//     ReadSession::Spec::ReadAllFromPath(fixture_path));

//   auto &reader = *rp;

  
//   std::vector<Entry> actual_entries;
//   bool still_reading = true;
//   do {
//     MaybeEntry maybe_next = reader.GetNext();
//     if (maybe_next.IsEndOfSequence()) {
//       still_reading = false;
//       break;
//     }

//     ASSERT_TRUE(maybe_next.IsOk()) << maybe_next.error;
    
//     actual_entries.push_back(*maybe_next.value);

//   } while(still_reading);

//   auto expected_entries = kExpectedEntries;

//   ASSERT_EQ(actual_entries.size(), expected_entries.size());
//   for (size_t i = 0; i < actual_entries.size(); ++i) {
//     auto expected = expected_entries[i];
//     auto actual = actual_entries[i];

//     EXPECT_EQ(
//       PBToString(actual.msg),
//       PBToString(expected.msg));
    

//     EXPECT_TRUE(actual.EntryDataEqualTo(expected)) << 
//       "Actual: " << actual.ToString() << 
//       "\nExpected:\n" << expected.ToString();
//   }
// }

// TODO: test with tar and zip