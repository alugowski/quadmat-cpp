// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include <functional>
#include <generators/tuple_generators.h>

#include "catch.hpp"

#include "quadmat.h"

using Catch::Matchers::Equals;

TEST_CASE("Dense String Matrix") {
    SECTION("empty matrix") {
        int size = 4;

        quadmat::dense_string_matrix smat({size, size});

        smat.fill_tuples(quadmat::simple_tuples_generator<double, int>::EmptyMatrix());

        string str = smat.to_string();

        string expected = "   \n"
                          "   \n"
                          "   \n"
                          "   ";

        REQUIRE(str == expected);
    }
    SECTION("small identity matrix") {
        int size = 4;
        quadmat::dense_string_matrix smat({size, size});
        quadmat::identity_tuples_generator<double, int> gen(size);

        smat.fill_tuples(gen);

        string str = smat.to_string();

        string expected = "1      \n"
                          "  1    \n"
                          "    1  \n"
                          "      1";

        REQUIRE(str == expected);
    }
    SECTION("Kepner-Gilbert graph") {
        quadmat::shape_t shape = quadmat::simple_tuples_generator<double, int>::KepnerGilbertGraph_shape();
        quadmat::dense_string_matrix smat(shape);

        smat.fill_tuples(quadmat::simple_tuples_generator<double, int>::KepnerGilbertGraph());

        string str = smat.to_string();

        string expected =
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

    SECTION("shuffle") {
        vector<int> vec(sorted);
        REQUIRE_THAT(vec, Equals(sorted));

        quadmat::stable_shuffle(std::begin(vec), std::end(vec));
        REQUIRE_THAT(vec, !Equals(sorted));
        vector<int> vec_shuffled(vec);

        // re-sort to make sure nothing went missing
        std::sort(std::begin(vec), std::end(vec), std::less<>());
        REQUIRE_THAT(vec, Equals(sorted));

        // re-shuffle to test stability
        quadmat::stable_shuffle(std::begin(vec), std::end(vec));
        REQUIRE_THAT(vec, Equals(vec_shuffled));
    }
}

TEST_CASE("Slicing") {
    int size = GENERATE(0, 7, 10);
    SECTION(std::string("vector size ") + std::to_string(size)) {

        vector<int> original(size);
        std::iota(original.begin(), original.end(), 0);

        int num_parts = GENERATE(1, 2, 3, 20);
        SECTION(std::to_string(num_parts) + " splits") {
            auto ranges = quadmat::slice_ranges(num_parts, begin(original), end(original));

            // make sure we have the right number of ranges
            int expected_num_parts = (size == 0 ? 1 : std::min(size, num_parts));
            REQUIRE(ranges.size() == expected_num_parts);

            // make sure sizes are right
            int total_range_size = std::accumulate(begin(ranges), end(ranges), 0,
                                                   [](int sum, auto r) { return sum + r.size(); });
            REQUIRE(total_range_size == size);

            // make sure the ranges cover the right elements
            vector<int> copy;
            for (auto range : ranges) {
                REQUIRE(range.size() >= size / (expected_num_parts + 1));
                REQUIRE(range.size() * (expected_num_parts - 1) <= size);

                copy.insert(copy.end(), begin(range), end(range));
            }

            REQUIRE_THAT(copy, Equals(original));
        }
    }
}

TEST_CASE("Numeric Utilities") {
    SECTION("clear_all_except_msb") {
        REQUIRE(quadmat::clear_all_except_msb(0) == 0);
        REQUIRE(quadmat::clear_all_except_msb(1) == 1);
        REQUIRE(quadmat::clear_all_except_msb(2) == 2);
        REQUIRE(quadmat::clear_all_except_msb(3) == 2);
        REQUIRE(quadmat::clear_all_except_msb(0b100111000) == 0b100000000);
        REQUIRE(quadmat::clear_all_except_msb(0b111111000) == 0b100000000);
        REQUIRE(quadmat::clear_all_except_msb(0b111111111) == 0b100000000);
        REQUIRE(quadmat::clear_all_except_msb((1ul << 63ul) - 1ul) == (1ul << 62ul));
        REQUIRE(quadmat::clear_all_except_msb(std::numeric_limits<quadmat::index_t>::max()) == (1ul << 62ul));
    }
}