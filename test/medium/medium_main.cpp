// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_MAIN
#include "../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

TEST_CASE("Matrix Generation"){
matrix<double> m = quadmat::identity<double>(10);
REQUIRE(m.get_shape() == shape_t{10, 10});
}
