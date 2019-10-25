// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat.h"

TEST_CASE("SpA") {
    SECTION("sparse basic") {
        int size = 10;
        quadmat::identity_tuples_generator<double, int> gen(size);

        quadmat::sparse_spa<double, int> spa(size);

        // fill spa
        for (auto tup : gen) {
            spa.update(std::get<0>(tup), std::get<2>(tup));
        }

        // test contents
        int count = 0;
        for (auto pair : spa) {
            REQUIRE(pair.first == count);
            REQUIRE(pair.second == 1);
            count++;
        }
        REQUIRE(count == size);

        // add again
        for (auto tup : gen) {
            spa.update(std::get<0>(tup), std::get<2>(tup));
        }

        // test contents
        count = 0;
        for (auto pair : spa) {
            REQUIRE(pair.first == count);
            REQUIRE(pair.second == 2);
            count++;
        }
        REQUIRE(count == size);

        // clear
        spa.clear();
        count = 0;
        for (auto pair : spa) {
            count++;
        }
        REQUIRE(count == 0);
    }
}