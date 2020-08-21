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
