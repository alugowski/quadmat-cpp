// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include <cstdint>

#include "quadmat/quadmat.h"

using namespace quadmat;


TEST_CASE("Tuples Generator") {
    SECTION("Identity Matrix") {
        SECTION("basic") {
            int size = 10;
            IdentityTuplesGenerator<double, int> gen(size);

            int count = 0;
            for (auto tup : gen) {
                REQUIRE(std::get<0>(tup) == count);
                REQUIRE(std::get<1>(tup) == count);
                REQUIRE(std::get<2>(tup) == 1);
                count++;
            }

            REQUIRE(count == size);
        } SECTION("overflow safety") {
            IdentityTuplesGenerator<double, int8_t> gen(0, INT8_MAX);

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
    SECTION("Full Matrix") {
        int size = 5;
        FullTuplesGenerator<double, int> gen({size, size}, 1);

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
