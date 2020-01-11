// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_MAIN
#include "../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

TEST_CASE("Matrix Construction"){
    Matrix<double> m({10, 20});
    REQUIRE(m.GetShape() == Shape{10, 20});
}

TEST_CASE("Matrix Generation"){
    Matrix<double> m = Identity<double>(10);
    REQUIRE(m.GetShape() == Shape{10, 10});
}
