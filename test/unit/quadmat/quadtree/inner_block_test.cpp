// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

TEST_CASE("Inner Block"){
    SECTION("constructor") {
        REQUIRE_THROWS(InnerBlock<double>(0));
        REQUIRE_THROWS(InnerBlock<double>(3));
        REQUIRE_NOTHROW(InnerBlock<double>(4));
    }
    SECTION("children") {
        REQUIRE(InnerBlock<double>(4).GetNumChildren() == 4);
        REQUIRE_THROWS(InnerBlock<double>(4).GetChildOffsets(5, {}));
        REQUIRE_THROWS(InnerBlock<double>(4).GetChildShape(5, {}));
    }
    SECTION("size") {
        auto size = InnerBlock<double>(4).GetSize();
        REQUIRE(size.index_bytes == 0);
        REQUIRE(size.value_bytes == 0);
        REQUIRE(size.overhead_bytes > 0);
    }
}
