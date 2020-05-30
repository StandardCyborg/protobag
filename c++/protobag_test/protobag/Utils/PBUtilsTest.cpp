#include "gtest/gtest.h"

#include <string>

#include "protobag/Utils/PBUtils.hpp"
#include "protobag_msg/ProtobagMsg.pb.h"

using namespace protobag;

TEST(PBUtilsTest, TestPBFactoryBasicSerialization) {
  StdMsg_String msg;
  msg.set_value("foo");

  static const std::string kMsgPrototxt = "value: \"foo\"\n";

  static const std::string kMsgProtobin = "\n\x3" "foo";

  {
    auto maybe_pbtxt = PBFactory::ToTextFormatString(msg);
    ASSERT_TRUE(maybe_pbtxt.IsOk()) << maybe_pbtxt.error;
    EXPECT_EQ(*maybe_pbtxt.value, kMsgPrototxt);
  }

  {
    auto maybe_pb = PBFactory::ToBinaryString(msg);
    ASSERT_TRUE(maybe_pb.IsOk()) << maybe_pb.error;
    EXPECT_EQ(*maybe_pb.value, kMsgProtobin);
  }

  {
    auto maybe_msg = PBFactory::LoadFromContainer<StdMsg_String>(kMsgPrototxt);
    ASSERT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
    EXPECT_EQ(maybe_msg.value->value(), "foo");
  }

  {
    auto maybe_msg = PBFactory::LoadFromContainer<StdMsg_Int>(kMsgPrototxt);
    EXPECT_TRUE(!maybe_msg.IsOk());
    EXPECT_EQ(
      maybe_msg.error,
      "Failed to read a type.googleapis.com/protobag.StdMsg.Int");
  }

  {
    std::string s = "garbage";
    auto maybe_msg = PBFactory::LoadFromContainer<StdMsg_String>(s);
    EXPECT_TRUE(!maybe_msg.IsOk());
    EXPECT_EQ(
      maybe_msg.error,
      "Failed to read a type.googleapis.com/protobag.StdMsg.String");
  }

}

// TODO MORE TEST PBFACTORY

static const std::string kTestDynamicMsgFactoryBasicExpectedStr = (
"basdgasg"
);

TEST(PBUtilsTest, TestDynamicMsgFactoryBasic) {
  StdMsg_String msg;
  msg.set_value("foo");
  auto msg_text_format = PBToString(msg);

  static const std::string kMsgPrototxt = "value: \"foo\"\n";
  ASSERT_EQ(msg_text_format, kMsgPrototxt);

  {
    DynamicMsgFactory factory;

    {
      ::google::protobuf::FileDescriptorProto fd_proto;
      msg.GetDescriptor()->file()->CopyTo(&fd_proto);
      factory.RegisterType(fd_proto);
    }

    EXPECT_EQ(factory.ToString(), kTestDynamicMsgFactoryBasicExpectedStr);

    {
      auto maybe_msgp = factory.LoadFromContainer(
            GetTypeURL<StdMsg_String>(),
            msg_text_format);
      ASSERT_TRUE(maybe_msgp.IsOk()) << maybe_msgp.error;

      auto msgp = std::move(*maybe_msgp.value);
      ASSERT_TRUE(msgp);

      EXPECT_EQ(msgp->GetTypeName(), "moof");
    }
  }
}
