// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using namespace quadmat;

using Catch::Matchers::Equals;

TEMPLATE_TEST_CASE("SpA", "", // NOLINT(cert-err58-cpp)
        (DenseSpa<Index, PlusTimesSemiring<double>>),
        (MapSpa<Index, PlusTimesSemiring<double>>)) {

    SECTION("Basic") {
        int size = 10;

        std::vector<Index> original_rows(size);
        std::iota(original_rows.begin(), original_rows.end(), 0);

        std::vector<double> original_values(size, 1.0);
        std::vector<double> doubled_values(size, 2.0);

        TestType spa(size);

        // fill spa
        spa.Scatter(begin(original_rows), end(original_rows), begin(original_values));

        // test contents
        {
            std::vector<Index> test_rows;
            std::vector<double> test_values;
            spa.Gather(test_rows, test_values);

            REQUIRE_THAT(test_rows, Equals(original_rows));
            REQUIRE_THAT(test_values, Equals(original_values));
        }

        // add again
        spa.Scatter(begin(original_rows), end(original_rows), begin(original_values));

        // test contents, values should be doubled
        {
            std::vector<Index> test_rows;
            std::vector<double> test_values;
            spa.Gather(test_rows, test_values);

            REQUIRE_THAT(test_rows, Equals(original_rows));
            REQUIRE_THAT(test_values, Equals(doubled_values));
        }

        // clear
        spa.Clear();

        // test contents, should be empty
        {
            std::vector<Index> test_rows;
            std::vector<double> test_values;
            spa.Gather(test_rows, test_values);

            REQUIRE(test_rows.empty());
            REQUIRE(test_values.empty());
        }

        // fill spa while doubling
        spa.Scatter(begin(original_rows), end(original_rows), begin(original_values), 2);

        // test contents, values should be doubled
        {
            std::vector<Index> test_rows;
            std::vector<double> test_values;
            spa.Gather(test_rows, test_values);

            REQUIRE_THAT(test_rows, Equals(original_rows));
            REQUIRE_THAT(test_values, Equals(doubled_values));
        }
    }
}