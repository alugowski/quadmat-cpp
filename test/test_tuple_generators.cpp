// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat.h"

#include <cstdint>

TEST_CASE("Identity Tuples") {
    SECTION("basic") {
        int size = 10;
        quadmat::identity_tuples_generator<double, int> gen(size);

        int count = 0;
        for (auto tup : gen) {
            REQUIRE(std::get<0>(tup) == count);
            REQUIRE(std::get<1>(tup) == count);
            REQUIRE(std::get<2>(tup) == 1);
            count++;
        }

        REQUIRE(count == size);
    }
    SECTION("overflow safety") {
        quadmat::identity_tuples_generator<double, int8_t> gen(0, INT8_MAX);

        int count = 0;
        for (auto tup : gen) {
            REQUIRE(std::get<0>(tup) == count);
            REQUIRE(std::get<1>(tup) == count);
            REQUIRE(std::get<2>(tup) == 1);
            count++;
        }

        REQUIRE(count == INT8_MAX + 1);
    }
}
