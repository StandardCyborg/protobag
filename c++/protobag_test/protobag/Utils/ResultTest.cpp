#include "gtest/gtest.h"

#include "protobag/Utils/Result.hpp"

using namespace protobag;

Result<int> Ok() {
  return Result<int>::Ok(1337);
}

Result<int> Fail() {
  return Result<int>::Err("foo");
}

TEST(ResultTest, TestOk) {
  auto res = Ok();
  EXPECT_TRUE(res.IsOk());
  EXPECT_EQ(*res.value, 1337);
}

TEST(ResultTest, TestFail) {
  auto res = Fail();
  EXPECT_FALSE(res.IsOk());
  EXPECT_EQ(res.error, "foo");
}
