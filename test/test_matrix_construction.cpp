// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "quadmat.h"

using quadmat::matrix;

TEST_CASE("Matrix Construction"){
    matrix<double> m({10, 20});
    REQUIRE(m.get_shape() == quadmat::shape_t{10, 20});
}

TEST_CASE("Matrix Generation"){
    matrix<double> m = quadmat::identity<double>(10);
    REQUIRE(m.get_shape() == quadmat::shape_t{10, 10});
}
