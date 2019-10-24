// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat.h"

TEST_CASE("Triples Block") {
    SECTION("basic construction") {
        int size = 10;
        quadmat::identity_tuples_generator<double, int> gen(size);

        quadmat::triples_block<double, int> block(size, size);
        block.add(gen);

        SECTION("tuples") {
            int count = 0;
            for (auto tup : block) {
                REQUIRE(std::get<0>(tup) == count);
                REQUIRE(std::get<1>(tup) == count);
                REQUIRE(std::get<2>(tup) == 1);
                count++;
            }

            REQUIRE(count == size);
        }

        SECTION("sorted tuples") {
            int count = 0;
            for (auto tup : block.sorted_range()) {
                REQUIRE(std::get<0>(tup) == count);
                REQUIRE(std::get<1>(tup) == count);
                REQUIRE(std::get<2>(tup) == 1);
                count++;
            }

            REQUIRE(count == size);
        }
    }
}
