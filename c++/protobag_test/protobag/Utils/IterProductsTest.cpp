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

#include <algorithm>

#include "protobag/Utils/IterProducts.hpp"

using namespace protobag;

typedef std::vector<std::vector<size_t>> product_list;
void CheckExpectedProducts(IterProducts &i, product_list expected) {
  product_list actual;
  auto next = i.GetNext();
  while (!next.IsEndOfSequence()) {
    actual.push_back(next.indices);
    next = i.GetNext();
  }

  EXPECT_EQ(expected.size(), actual.size());
  
  std::sort(expected.begin(), expected.end());
  std::sort(actual.begin(), actual.end());
  EXPECT_EQ(expected, actual);
}

TEST(IterProductsTest, TestEmpty) {
  IterProducts i({});
  auto next = i.GetNext();
  ASSERT_TRUE(next.IsEndOfSequence()) << ::testing::PrintToString(next.indices);
}

TEST(IterProductsTest, TestHasPoolWithZero) {
  {
    IterProducts i({0});
    auto next = i.GetNext();
    ASSERT_TRUE(next.IsEndOfSequence()) << 
      ::testing::PrintToString(next.indices);
  }
  {
    IterProducts i({1, 0, 1});
    auto next = i.GetNext();
    ASSERT_TRUE(next.IsEndOfSequence()) << 
      ::testing::PrintToString(next.indices);
  }
}

TEST(IterProductsTest, TestOnePoolBasic) {
  IterProducts i({1});
  {
    auto next = i.GetNext();
    ASSERT_TRUE(!next.IsEndOfSequence());
    EXPECT_EQ(next.indices, std::vector<size_t>{0});
  }

  {
    auto next = i.GetNext();
    ASSERT_TRUE(next.IsEndOfSequence()) << 
      ::testing::PrintToString(next.indices);
  }

  // Should still be end of sequence
  {
    auto next = i.GetNext();
    ASSERT_TRUE(next.IsEndOfSequence()) << 
      ::testing::PrintToString(next.indices);
  }
}

TEST(IterProductsTest, TestOnePoolLong) {
  static const size_t N = 10;
  IterProducts i({N});

  product_list expected_products;
  for (size_t r = 0; r < N; ++r) {
    expected_products.push_back({r});
  }

  CheckExpectedProducts(i, expected_products);
}

TEST(IterProductsTest, TestTwoPoolsLong) {
  static const size_t p1_N = 10;
  static const size_t p2_N = 5;
  IterProducts i({p1_N, p2_N});

  product_list expected_products;
  for (size_t p1i = 0; p1i < p1_N; ++p1i) {
    for (size_t p2i = 0; p2i < p2_N; ++p2i) {
      expected_products.push_back({p1i, p2i});
    }
  }

  CheckExpectedProducts(i, expected_products);
}

TEST(IterProductsTest, TestNBinaryPools) {
  static const size_t NUM_POOLS = 3;

  // N size-2 pools -> the set of all products is the powerset of
  // N binary variables
  product_list expected_products;
  for (size_t product = 0; product < (1 << NUM_POOLS); ++product) {
    std::vector<size_t> expected_indices(NUM_POOLS, 0);
    for (size_t pool = 0; pool < NUM_POOLS; ++pool) {
      if (product & (1 << pool)) {
        expected_indices[pool] = 1;
      }
    }
    expected_products.push_back(expected_indices);
  }

  IterProducts i(std::vector<size_t>(NUM_POOLS, 2));
  CheckExpectedProducts(i, expected_products);
}

TEST(IterProductsTest, Test7PoolsSize5) {
  IterProducts i(std::vector<size_t>(7, 5));
  static const size_t expected_num_products = 5*5*5*5*5*5*5;

  size_t actual_num_products = 0;
  
  auto next = i.GetNext();
  while (!next.IsEndOfSequence()) {
    actual_num_products += 1;
    next = i.GetNext();
  }

  EXPECT_EQ(expected_num_products, actual_num_products);
}
