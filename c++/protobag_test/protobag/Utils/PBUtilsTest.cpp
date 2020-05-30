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
      "Failed to read a protobag.StdMsg.Int");
  }

  {
    std::string s = "garbage";
    auto maybe_msg = PBFactory::LoadFromContainer<StdMsg_String>(s);
    EXPECT_TRUE(!maybe_msg.IsOk());
    EXPECT_EQ(
      maybe_msg.error,
      "Failed to read a protobag.StdMsg.String");
  }

}

// TODO MORE TEST PBFACTORY



/******************************************************************************

The test TestDynamicMsgFactoryBasic below uses a type `Moof` for which we
do NOT generate protobuf-generated-C++ code in order to exercise purely
dynamic message creation.

To re-generate the fixture data below, use the `print_fd.py` script
and see other artifacts included at
c++/protobag_test/fixtures/PBUtilsTest.TestDynamicMsgFactoryBasic

The fixtures below include:
 * A message instance in protobuf text format
 * The FileDescriptorProto instance for the message, in protobuf text format
 * The expected state of a DynamicMsgFactory after registering the message type

******************************************************************************/

static const std::string kTestDynamicMsgFactoryBasic_Msg_Prototxt = 
R"(x: "i am a dogcow" )";

static const std::string kTestDynamicMsgFactoryBasic_FileDescriptorProto_Prototxt = 
R"(name: "moof.proto"
package: "my_package"
message_type {
  name: "Moof"
  field {
    name: "x"
    number: 1
    label: LABEL_OPTIONAL
    type: TYPE_STRING
  }
}
syntax: "proto3")";

static const std::string kTestDynamicMsgFactoryBasicExpectedStr = 
R"(DynamicMsgFactory
Factory known types:
my_package.Moof

DB known filenames:
moof.proto

)";

TEST(PBUtilsTest, TestDynamicMsgFactoryBasic) {
  DynamicMsgFactory factory;

  {
    auto maybe_fd_msg = 
      PBFactory::LoadFromContainer<::google::protobuf::FileDescriptorProto>(
        kTestDynamicMsgFactoryBasic_FileDescriptorProto_Prototxt);
    ASSERT_TRUE(maybe_fd_msg.IsOk()) << maybe_fd_msg.error;
    factory.RegisterType(*maybe_fd_msg.value);
  }

  EXPECT_EQ(factory.ToString(), kTestDynamicMsgFactoryBasicExpectedStr);

  {
    auto maybe_msgp = factory.LoadFromContainer(
          "my_package.Moof",
          kTestDynamicMsgFactoryBasic_Msg_Prototxt);
    ASSERT_TRUE(maybe_msgp.IsOk()) << maybe_msgp.error;

    auto msgp = std::move(*maybe_msgp.value);
    ASSERT_TRUE(msgp);

    EXPECT_EQ(msgp->GetTypeName(), "my_package.Moof");
    EXPECT_EQ(msgp->DebugString(), "x: \"i am a dogcow\"\n");
  }

}
