// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "quadmat.h"

using quadmat::matrix;

TEST_CASE("Matrix Construction"){
    matrix<double> m(10, 20);
    REQUIRE(m.get_nrow() == 10);
    REQUIRE(m.get_ncol() == 20);
}

TEST_CASE("Matrix generation"){
    matrix<double> m = quadmat::identity<double>(10);
    REQUIRE(m.get_nrow() == 10);
    REQUIRE(m.get_ncol() == 10);
}
