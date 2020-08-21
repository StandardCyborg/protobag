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
R"(x: "i am a dogcow"
inner {
  inner_v: 1337
}
)";

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
  field {
    name: "inner"
    number: 2
    label: LABEL_OPTIONAL
    type: TYPE_MESSAGE
    type_name: ".my_package.Moof.InnerMoof"
  }
  nested_type {
    name: "InnerMoof"
    field {
      name: "inner_v"
      number: 1
      label: LABEL_OPTIONAL
      type: TYPE_INT64
    }
  }
}
syntax: "proto3"

)";

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
    
    // Use protobuf build-in reflection API
    {
      using namespace ::google::protobuf;
      const Descriptor* descriptor = msgp->GetDescriptor();
      ASSERT_TRUE(descriptor);

      const FieldDescriptor* x_field = descriptor->FindFieldByName("x");
      ASSERT_TRUE(x_field);
      EXPECT_EQ(x_field->type(), FieldDescriptor::TYPE_STRING);
      
      const Reflection* reflection = msgp->GetReflection();
      ASSERT_TRUE(reflection);
      EXPECT_EQ(reflection->GetString(*msgp, x_field), "i am a dogcow");
    }

    // Use Protobag Utils
    {
      // Hit attribute `x`
      {
        auto maybe_v = GetAttr_string(msgp.get(), "x");
        ASSERT_TRUE(maybe_v.IsOk()) << maybe_v.error;
        EXPECT_EQ(*maybe_v.value, "i am a dogcow");
      }

      {
        auto maybe_v = GetDeep_string(msgp.get(), "x");
        ASSERT_TRUE(maybe_v.IsOk()) << maybe_v.error;
        EXPECT_EQ(*maybe_v.value, "i am a dogcow");
      }
      
      // Error attribute `x`
      {
        auto maybe_v = GetAttr_string(msgp.get(), "does_not_exist");
        ASSERT_TRUE(!maybe_v.IsOk());
        EXPECT_EQ(
          maybe_v.error, "Msg my_package.Moof has no field does_not_exist");
      }

      {
        auto maybe_v = GetDeep_string(msgp.get(), "does_not_exist");
        ASSERT_TRUE(!maybe_v.IsOk());
        EXPECT_EQ(
          maybe_v.error, "Msg my_package.Moof has no field does_not_exist");
      }

      {
        auto maybe_v = GetDeep_string(msgp.get(), "");
        ASSERT_TRUE(!maybe_v.IsOk());
        EXPECT_EQ(maybe_v.error, "Msg my_package.Moof has no field ");
      }
      
      {
        auto maybe_v = GetAttr_int32(msgp.get(), "x");
        ASSERT_TRUE(!maybe_v.IsOk());
        EXPECT_EQ(
          maybe_v.error,
          "Wanted field x on msg my_package.Moof to be type double, but my_package.Moof.x is of type string");
      }

      {
        auto maybe_v = GetDeep_int32(msgp.get(), "x");
        ASSERT_TRUE(!maybe_v.IsOk());
        EXPECT_EQ(
          maybe_v.error,
          "Wanted field x on msg my_package.Moof to be type double, but my_package.Moof.x is of type string");
      }

      // Hit nested message
      {
        auto maybe_v = GetAttr_msg(msgp.get(), "inner");
        ASSERT_TRUE(maybe_v.IsOk()) << maybe_v.error;
        auto inner_msgp = *maybe_v.value;
        ASSERT_TRUE(inner_msgp);
        EXPECT_EQ(inner_msgp->DebugString(), "inner_v: 1337\n");
      }

      // Hit nested message value
      {
        auto maybe_v = GetDeep_int64(msgp.get(), "inner.inner_v");
        ASSERT_TRUE(maybe_v.IsOk()) << maybe_v.error;
        EXPECT_EQ(*maybe_v.value, 1337);
      }
    }
  }

}
