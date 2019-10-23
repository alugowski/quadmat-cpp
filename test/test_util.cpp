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
    vector<size_t> permutation = quadmat::get_sort_permutation(unsorted.begin(), unsorted.end(), std::less<>());

    SECTION("out of place permutation") {
        vector<int> permute_sorted = quadmat::apply_permutation(unsorted, permutation);
        REQUIRE_THAT(permute_sorted, Equals(sorted));
    }
    SECTION("in-place permutation") {
        SECTION("vector") {
            vector<int> vec(unsorted);
            quadmat::apply_permutation_inplace(vec, permutation);
            REQUIRE_THAT(vec, Equals(sorted));
        }
        SECTION("range single") {
            vector<int> vec(unsorted);
            quadmat::apply_permutation_inplace(permutation, vec.begin());
            REQUIRE_THAT(vec, Equals(sorted));
        }
        SECTION("range multi") {
            vector<int> vec(unsorted);
            vector<int> vec2(unsorted);
            quadmat::apply_permutation_inplace(permutation, vec.begin(), vec2.begin());
            REQUIRE_THAT(vec, Equals(sorted));
            REQUIRE_THAT(vec2, Equals(sorted));
        }
    }
}
