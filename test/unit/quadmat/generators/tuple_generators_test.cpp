// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include <cstdint>

TEST_CASE("Tuples Generator") {
    SECTION("identity") {
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
        }SECTION("overflow safety") {
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
    SECTION("full") {
        int size = 5;
        quadmat::full_tuples_generator<double, int> gen({size, size}, 1);

        int count = 0;
        int expected_row = 0;
        int expected_col = 0;
        for (auto tup : gen) {
            REQUIRE(std::get<0>(tup) == expected_row);
            REQUIRE(std::get<1>(tup) == expected_col);
            REQUIRE(std::get<2>(tup) == 1);

            count++;
            expected_row = (expected_row + 1) % size;
            expected_col = count / size;
        }

        REQUIRE(count == size*size);
    }
}
