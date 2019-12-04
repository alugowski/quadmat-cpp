// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

TEST_CASE("Block Container"){
    SECTION("single_block_container") {
        SECTION("discriminating bit") {
            REQUIRE(single_block_container<double>({0, 0}).get_discriminating_bit() == 1);
            REQUIRE(single_block_container<double>({1, 1}).get_discriminating_bit() == 1);
            REQUIRE(single_block_container<double>({7, 7}).get_discriminating_bit() == 8);
            REQUIRE(single_block_container<double>({8, 8}).get_discriminating_bit() == 8);
            REQUIRE(single_block_container<double>({9, 9}).get_discriminating_bit() == 16);
        }
    }
}
