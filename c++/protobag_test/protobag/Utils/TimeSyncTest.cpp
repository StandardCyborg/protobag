/*
Copyright 2020 Standard Cyborg

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "gtest/gtest.h"

#include "protobag/Entry.hpp"
#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/StdMsgUtils.hpp"
#include "protobag/Utils/TimeSync.hpp"
#include "protobag/ReadSession.hpp"

#include "protobag_test/Utils.hpp"

using namespace protobag;
using namespace protobag_test;


namespace protobag {

inline
bool operator==(const Entry &lhs, const Entry &rhs) {
  const auto lhstt = lhs.GetTopicTime();
  const auto rhstt = rhs.GetTopicTime();
  return 
    lhstt.has_value() == rhstt.has_value() &&
    (lhstt.has_value() || (
      lhstt->topic() == rhstt->topic() &&
      lhstt->timestamp().seconds() == rhstt->timestamp().seconds() &&
      lhstt->timestamp().nanos() == rhstt->timestamp().nanos()));
}

inline
std::ostream& operator<<(std::ostream& os, const Entry &entry) {
  // os << entry.ToString();
  os << "tt: " << PBToString(*entry.GetTopicTime(), false) << std::endl;
  // os << "data: " << PBToString(entry.msg) << std::endl;
  return os;
}

} /* namespace protobag */


std::list<Entry> Flatten(const std::list<EntryBundle> &bundles) {
  std::list<Entry> entries;
  for (const auto &bundle : bundles) {
    for (const auto &entry : bundle) {
      entries.push_back(entry);
    }
  }
  return entries;
}

std::list<EntryBundle> ConsumeBundles(const TimeSync::Ptr &sync) {
  if (!sync) { throw std::runtime_error("null sync"); }

  std::list<EntryBundle> bundles;
  bool reading = true;
  while (reading) {
    auto maybe_bundle = sync->GetNext();
    if (maybe_bundle.IsOk()) {
      bundles.push_back(*maybe_bundle.value);
    } else if (maybe_bundle.IsEndOfSequence()) {
      reading = false;
    } else {
      EXPECT_TRUE(false) << "Error while reading: " << maybe_bundle.error;
      reading = false;
    }
  }

  return bundles;
}

TEST(TimeSyncTest, TestMaxSlopSyncBasic) {
  static const std::list<EntryBundle> kExpectedBundles = {
    {
      Entry::CreateStamped("/topic1", 0, 0, ToStringMsg("foo")),
      Entry::CreateStamped("/topic2", 0, 0, ToIntMsg(1337)),
    },

    {
      Entry::CreateStamped("/topic1", 1, 0, ToStringMsg("foo")),
      Entry::CreateStamped("/topic2", 1, 0, ToIntMsg(1337)),
    },

    {
      Entry::CreateStamped("/topic1", 2, 0, ToStringMsg("foo")),
      Entry::CreateStamped("/topic2", 2, 0, ToIntMsg(1337)),
    },
  };

  protobag::Selection sel;
  sel.mutable_window();
  auto fixture = CreateInMemoryReadSession(
    sel,
    Flatten(kExpectedBundles));

  auto maybeSync = MaxSlopTimeSync::Create(
    fixture,
    {
      .topics = {"/topic1", "/topic2"},
      .max_slop = SecondsToDuration(0.5),
    });
  ASSERT_TRUE(maybeSync.IsOk()) << maybeSync.error;

  auto actual_bundles = ConsumeBundles(*maybeSync.value);

  EXPECT_EQ(kExpectedBundles, actual_bundles);
}

