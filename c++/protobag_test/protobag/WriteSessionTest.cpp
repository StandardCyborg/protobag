#include "gtest/gtest.h"

#include <exception>
#include <vector>

#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/StdMsgUtils.hpp"
#include "protobag/WriteSession.hpp"

#include "protobag_test/Utils.hpp"

using namespace protobag;
using namespace protobag_test;

std::vector<Entry> CreateEntriesFixture() {
  return {
    Entry::CreateStamped("/topic1", 0, 0, ToStringMsg("foo")),
    Entry::CreateStamped("/topic1", 1, 0, ToStringMsg("bar")),
    Entry::CreateStamped("/topic2", 0, 0, ToIntMsg(1337)),
  };
}

inline 
WriteSession::Ptr OpenWriterAndCheck(const WriteSession::Spec &spec) {
  auto result = WriteSession::Create(spec);
  if (!result.IsOk()) {
    throw std::runtime_error(result.error);
  }

  auto w = *result.value;
  if (!w) {
    throw std::runtime_error("Null pointer exception: bad result object");
  }

  return w;
}

inline
void ExpectWriteOk(WriteSession &w, const Entry &entry) {
  OkOrErr result = w.WriteEntry(entry);
  if (!result.IsOk()) {
    throw new std::runtime_error(result.error);
  }
}

TEST(WriteSessionDirectory, TestBasic) {
  auto testdir = CreateTempDir("WriteSessionDirectory.TestBasic");

  {
    auto wp = OpenWriterAndCheck({
      .archive_spec = {
        .mode="write",
        .path=testdir,
        .format="directory",
      }
    });

    auto &writer = *wp;

    for (const auto &entry : CreateEntriesFixture()) {
      ExpectWriteOk(writer, entry);
    }

    // writer auto-closes and writes meta
  }

  // Now check what we wrote
  {
    auto dar = OpenAndCheck({
      .mode="read",
      .path=testdir,
      .format="directory",
    });

    {
      auto namelist = dar->GetNamelist();
      std::vector<std::string> actual;
      for (auto name : namelist) {
        if (!IsProtoBagIndexTopic(name)) {
          actual.push_back(name);
        }
      }
      
      std::vector<std::string> expected = {
        "/topic1/1.protobin",
        "/topic1/2.protobin",
        "/topic2/1.protobin",
      };
      EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);
    }

    {
      auto res = dar->ReadAsStr("topic1/1.protobin");
      EXPECT_TRUE(res.IsOk()) << res.error;
      auto maybe_msg = PBFactory::LoadFromContainer<StampedMessage>(*res.value);
      EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const StampedMessage &m = *maybe_msg.value;
      EXPECT_EQ(m.timestamp().seconds(), 0);
      EXPECT_EQ(m.timestamp().nanos(), 0);
      EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.String");

      {
        auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_String>(m.msg());
        EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
        EXPECT_EQ(maybe_msg.value->value(), "foo");
      }
    }
    
    {
      auto res = dar->ReadAsStr("topic1/2.protobin");
      EXPECT_TRUE(res.IsOk()) << res.error;
      auto maybe_msg = PBFactory::LoadFromContainer<StampedMessage>(*res.value);
      EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const StampedMessage &m = *maybe_msg.value;
      EXPECT_EQ(m.timestamp().seconds(), 1);
      EXPECT_EQ(m.timestamp().nanos(), 0);
      EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.String");

      {
        auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_String>(m.msg());
        EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
        EXPECT_EQ(maybe_msg.value->value(), "bar");
      }
    }

    {
      auto res = dar->ReadAsStr("topic2/1.protobin");
      EXPECT_TRUE(res.IsOk()) << res.error;
      auto maybe_msg = PBFactory::LoadFromContainer<StampedMessage>(*res.value);
      EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const StampedMessage &m = *maybe_msg.value;
      EXPECT_EQ(m.timestamp().seconds(), 0);
      EXPECT_EQ(m.timestamp().nanos(), 0);
      EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.Int");

      {
        auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_Int>(m.msg());
        EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
        EXPECT_EQ(maybe_msg.value->value(), 1337);
      }
    }

    {
      // TODO check bag meta ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    }
  }

}
