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