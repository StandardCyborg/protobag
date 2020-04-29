//
//  main.m
//  ProtobagOSX
//
//  Created by Paul Wais on 4/29/20.
//  Copyright © 2020 Standard Cyborg. All rights reserved.
//

#include <iostream>
#include <gtest/gtest.h>

#include <protobag/DemoTest.cpp>

int main(int argc, char * argv[]) {
//    std::cout << "hi" << std::endl;
//    return 0;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
