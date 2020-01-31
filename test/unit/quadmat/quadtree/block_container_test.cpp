// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

TEST_CASE("Block Container"){
    SECTION("single_block_container") {
        SECTION("discriminating bit") {
            REQUIRE(SingleBlockContainer<double>({0, 0}).GetDiscriminatingBit() == 1);
            REQUIRE(SingleBlockContainer<double>({1, 1}).GetDiscriminatingBit() == 1);
            REQUIRE(SingleBlockContainer<double>({7, 7}).GetDiscriminatingBit() == 8);
            REQUIRE(SingleBlockContainer<double>({8, 8}).GetDiscriminatingBit() == 8);
            REQUIRE(SingleBlockContainer<double>({9, 9}).GetDiscriminatingBit() == 16);
        }
        SECTION("children") {
            REQUIRE(SingleBlockContainer<double>({1, 1}).GetNumChildren() == 1);
            REQUIRE(SingleBlockContainer<double>({1, 1}).GetChildOffsets(5, {}) == Offset{});
            REQUIRE(SingleBlockContainer<double>({1, 1}).GetChildShape(5, {}) == Shape{});
        }
    }
}
