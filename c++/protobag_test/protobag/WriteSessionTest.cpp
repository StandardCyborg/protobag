#include "gtest/gtest.h"

#include <exception>
#include <vector>

#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/StdMsgUtils.hpp"
#include "protobag/Utils/TopicTime.hpp"
#include "protobag/WriteSession.hpp"

#include "protobag_test/Utils.hpp"

using namespace protobag;
using namespace protobag_test;

std::vector<Entry> CreateEntriesFixture() {
  return {
    Entry::Create("/moof", ToStringMsg("moof")),

    Entry::CreateStamped("/topic1", 0, 0, ToStringMsg("foo")),
    Entry::CreateStamped("/topic1", 1, 0, ToStringMsg("bar")),
    Entry::CreateStamped("/topic2", 0, 0, ToIntMsg(1337)),

    Entry::CreateRawFromBytes("/i_am_raw", "i am raw data"),
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
    throw std::runtime_error(result.error);
  }
}

TEST(WriteSessionDirectory, TestBasic) {
  auto testdir = CreateTestTempdir("WriteSessionDirectory.TestBasic");

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
        "/topic1/0.0.stampedmsg.protobin",
        "/topic1/1.0.stampedmsg.protobin",
        "/topic2/0.0.stampedmsg.protobin",
        "/moof",
        "/i_am_raw",
      };
      EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);
    }


    //
    // Test very manual reads of the files we expect in place
    //

    {
      auto res = dar->ReadAsStr("topic1/0.0.stampedmsg.protobin");
      ASSERT_TRUE(res.IsOk()) << res.error;
      auto maybe_msg = PBFactory::LoadFromContainer<::google::protobuf::Any>(*res.value);
      ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const ::google::protobuf::Any &any_msg = *maybe_msg.value;
      ASSERT_EQ(any_msg.type_url(), GetTypeURL<StampedMessage>());
      {
        auto maybe_stamped = PBFactory::UnpackFromAny<StampedMessage>(any_msg);
        ASSERT_TRUE(maybe_stamped.IsOk()) << maybe_stamped.error;

        const StampedMessage &m = *maybe_stamped.value;
        EXPECT_EQ(m.timestamp().seconds(), 0);
        EXPECT_EQ(m.timestamp().nanos(), 0);
        EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.String");

        {
          auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_String>(m.msg());
          ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
          EXPECT_EQ(maybe_msg.value->value(), "foo");
        }
      }
    }
    

    {
      auto res = dar->ReadAsStr("topic1/1.0.stampedmsg.protobin");
      ASSERT_TRUE(res.IsOk()) << res.error;
      auto maybe_msg = PBFactory::LoadFromContainer<::google::protobuf::Any>(*res.value);
      ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const ::google::protobuf::Any &any_msg = *maybe_msg.value;
      ASSERT_EQ(any_msg.type_url(), GetTypeURL<StampedMessage>());
      {
        auto maybe_stamped = PBFactory::UnpackFromAny<StampedMessage>(any_msg);
        ASSERT_TRUE(maybe_stamped.IsOk()) << maybe_stamped.error;

        const StampedMessage &m = *maybe_stamped.value;
        EXPECT_EQ(m.timestamp().seconds(), 1);
        EXPECT_EQ(m.timestamp().nanos(), 0);
        EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.String");

        {
          auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_String>(m.msg());
          EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
          EXPECT_EQ(maybe_msg.value->value(), "bar");
        }
      }
    }


    {
      auto res = dar->ReadAsStr("topic2/0.0.stampedmsg.protobin");
      ASSERT_TRUE(res.IsOk()) << res.error;
      auto maybe_msg = PBFactory::LoadFromContainer<::google::protobuf::Any>(*res.value);
      ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const ::google::protobuf::Any &any_msg = *maybe_msg.value;
      ASSERT_EQ(any_msg.type_url(), GetTypeURL<StampedMessage>());
      {
        auto maybe_stamped = PBFactory::UnpackFromAny<StampedMessage>(any_msg);
        ASSERT_TRUE(maybe_stamped.IsOk()) << maybe_stamped.error;

        const StampedMessage &m = *maybe_stamped.value;
        EXPECT_EQ(m.timestamp().seconds(), 0);
        EXPECT_EQ(m.timestamp().nanos(), 0);
        EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.Int");

        {
          auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_Int>(m.msg());
          EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
          EXPECT_EQ(maybe_msg.value->value(), 1337);
        }
      }
    }

    {
      auto res = dar->ReadAsStr("moof");
      ASSERT_TRUE(res.IsOk()) << res.error;
      auto maybe_msg = PBFactory::LoadFromContainer<::google::protobuf::Any>(*res.value);
      ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const ::google::protobuf::Any &any_msg = *maybe_msg.value;
      ASSERT_EQ(any_msg.type_url(), GetTypeURL<StdMsg_String>());
      {
        auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_String>(any_msg);
        ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;

        const StdMsg_String &m = *maybe_msg.value;
        EXPECT_EQ(m.value(), "moof");
      }
    }

    {
      auto res = dar->ReadAsStr("i_am_raw");
      ASSERT_TRUE(res.IsOk()) << res.error;

      auto maybe_msg = PBFactory::LoadFromContainer<::google::protobuf::Any>(*res.value);
      ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
      const ::google::protobuf::Any &any_msg = *maybe_msg.value;
      ASSERT_EQ(any_msg.type_url(), "");
      ASSERT_EQ(any_msg.value(), "i am raw data");
    }

  }

}
