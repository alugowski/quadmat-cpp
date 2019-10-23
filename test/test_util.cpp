// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include <functional>

#include "catch.hpp"

#include "quadmat.h"

using Catch::Matchers::Equals;

TEST_CASE("Dense String Matrix") {
    SECTION("small identity matrix") {
        int size = 4;
        quadmat::dense_string_matrix smat(size, size);
        quadmat::identity_tuples_generator<double, int> gen(size);

        smat.fill_tuples(gen);

        string str = smat.to_string();

        string expected = "1      \n"
                          "  1    \n"
                          "    1  \n"
                          "      1";

        REQUIRE(str == expected);
    }
}

TEST_CASE("Vector Sorting and Permutation") {
    vector<int> unsorted = {8, 2, 5, 3, 5, 6, 1};
    vector<int> sorted(unsorted);
    std::sort(sorted.begin(), sorted.end(), std::less<>());

    // permute
    vector<size_t> permutation = quadmat::get_sort_permutation(unsorted, std::less<>());
    vector<int> permute_sorted = quadmat::permute(unsorted, permutation);

    // check
    REQUIRE_THAT(permute_sorted, Equals(sorted));
}
