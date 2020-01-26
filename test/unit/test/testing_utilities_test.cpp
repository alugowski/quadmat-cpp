// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include <functional>

#include "../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"
#include "../../test_utilities/testing_utilities.h"

using namespace quadmat;

using Catch::Matchers::Equals;

TEST_CASE("Dense String Matrix") {
    SECTION("empty matrix") {
        int size = 4;

        DenseStringMatrix smat({size, size});

        smat.FillTuples(SimpleTuplesGenerator<double, int>::GetEmptyTuples());

        std::string str = smat.ToString();

        std::string expected = "   \n"
                               "   \n"
                               "   \n"
                               "   ";

        REQUIRE(str == expected);
    }SECTION("small identity matrix") {
        int size = 4;
        DenseStringMatrix smat({size, size});
        IdentityTuplesGenerator<double, int> gen(size);

        smat.FillTuples(gen);

        std::string str = smat.ToString();

        std::string expected = "1      \n"
                               "  1    \n"
                               "    1  \n"
                               "      1";

        REQUIRE(str == expected);
    }SECTION("Kepner-Gilbert graph") {
        Shape shape = SimpleTuplesGenerator<double, int>::GetKepnerGilbertGraphShape();
        DenseStringMatrix smat(shape);

        smat.FillTuples(SimpleTuplesGenerator<double, int>::GetKepnerGilbertGraphTuples());

        std::string str = smat.ToString();

        std::string expected =
            "      1      \n"
            "1            \n"
            "      1   1 1\n"
            "1           1\n"
            "  1         1\n"
            "    1   1    \n"
            "  1          ";

        REQUIRE(str == expected);
    }
}
