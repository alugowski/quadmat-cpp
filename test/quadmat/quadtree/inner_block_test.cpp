// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "../../catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

TEST_CASE("Inner Block"){
    SECTION("constructor") {
        REQUIRE_THROWS(inner_block<double>(0));
        REQUIRE_THROWS(inner_block<double>(3));
        REQUIRE_NOTHROW(inner_block<double>(4));
    }
    SECTION("children") {
        REQUIRE(inner_block<double>(4).num_children() == 4);
        REQUIRE_THROWS(inner_block<double>(4).get_offsets(5, {}));
        REQUIRE_THROWS(inner_block<double>(4).get_child_shape(5, {}));
    }
    SECTION("size") {
        auto size = inner_block<double>(4).size();
        REQUIRE(size.index_bytes == 0);
        REQUIRE(size.value_bytes == 0);
        REQUIRE(size.overhead_bytes > 0);
    }
}
