// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

using Catch::Matchers::Equals;

TEMPLATE_TEST_CASE("SpA", "", // NOLINT(cert-err58-cpp)
        (quadmat::dense_spa<int, quadmat::plus_times_semiring<double>>),
        (quadmat::sparse_spa<int, quadmat::plus_times_semiring<double>>)) {

    SECTION("basic") {
        int size = 10;

        vector<int> original_rows(size);
        std::iota(original_rows.begin(), original_rows.end(), 0);

        vector<double> original_values(size, 1.0);
        vector<double> doubled_values(size, 2.0);

        TestType spa(size);

        // fill spa
        spa.update(begin(original_rows), end(original_rows), begin(original_values));

        // test contents
        {
            vector<int> test_rows;
            vector<double> test_values;
            spa.emplace_back_result(test_rows, test_values);

            REQUIRE_THAT(test_rows, Equals(original_rows));
            REQUIRE_THAT(test_values, Equals(original_values));
        }

        // add again
        spa.update(begin(original_rows), end(original_rows), begin(original_values));

        // test contents, values should be doubled
        {
            vector<int> test_rows;
            vector<double> test_values;
            spa.emplace_back_result(test_rows, test_values);

            REQUIRE_THAT(test_rows, Equals(original_rows));
            REQUIRE_THAT(test_values, Equals(doubled_values));
        }

        // clear
        spa.clear();

        // test contents, should be empty
        {
            vector<int> test_rows;
            vector<double> test_values;
            spa.emplace_back_result(test_rows, test_values);

            REQUIRE(test_rows.empty());
            REQUIRE(test_values.empty());
        }

        // fill spa while doubling
        spa.update(begin(original_rows), end(original_rows), begin(original_values), 2);

        // test contents, values should be doubled
        {
            vector<int> test_rows;
            vector<double> test_values;
            spa.emplace_back_result(test_rows, test_values);

            REQUIRE_THAT(test_rows, Equals(original_rows));
            REQUIRE_THAT(test_values, Equals(doubled_values));
        }
    }
}