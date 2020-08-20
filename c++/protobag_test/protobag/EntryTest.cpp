#include "gtest/gtest.h"

#include "protobag/Entry.hpp"
#include "protobag/Utils/StdMsgUtils.hpp"

using namespace protobag;

TEST(EntryTest, TestBasic) {

  auto entry = Entry::Create("/moof", ToStringMsg("moof"));
  EXPECT_EQ(entry.entryname, "/moof");
  EXPECT_EQ(entry.msg.type_url(), GetTypeURL<StdMsg_String>());
}
