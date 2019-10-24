// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat.h"

using quadmat::index_t;
using Catch::Matchers::Equals;

/**
 * Number of available canned matrices
 */
static const int get_problem_count = 3;

/**
 * Return a canned problem.
 * @return
 */
template <typename T, typename IT>
std::tuple<std::string, index_t, index_t, vector<std::tuple<IT, IT, T>>> get_problem(int which) {
    using ret_type = std::tuple<std::string, index_t, index_t, vector<std::tuple<IT, IT, T>>>;

    if (which == 0) {
        return ret_type("empty matrix", 10, 10,
                        quadmat::simple_tuples_generator<T, IT>::EmptyMatrix());
    } else if (which == 1) {
        quadmat::identity_tuples_generator<T, IT> gen(10);
        return ret_type("n=10 identity matrix", 10, 10,
                        vector<std::tuple<IT, IT, T>>(gen.begin(), gen.end()));
    } else if (which == 2) {
        int nrows, ncols;
        std::tie(nrows, ncols) = quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph_dim();

        return ret_type("Kepner-Gilbert graph", 7, 7,
                        quadmat::simple_tuples_generator<T, IT>::KepnerGilbertGraph());
    } else {
        // not allowed
        REQUIRE(which < get_problem_count);
        return ret_type("NOT ALLOWED", 10, 10,
                        quadmat::simple_tuples_generator<T, IT>::EmptyMatrix());
    }
}

TEST_CASE("DCSC Block") {
    SECTION("basic construction") {
        std::string description;
        int nrows, ncols;
        vector<std::tuple<int, int, double>> sorted_tuples;

        // get the problem
        int problem_num = GENERATE(range(0, get_problem_count));
        std::tie(description, nrows, ncols, sorted_tuples) = get_problem<double, int>(problem_num);

        SECTION(description) {
            quadmat::dcsc_block<double, int> block(nrows, ncols, sorted_tuples.size(), sorted_tuples);

            REQUIRE(block.get_nrows() == nrows);
            REQUIRE(block.get_ncols() == ncols);

            SECTION("get tuples back") {
                auto sorted_range = block.tuples();
                vector<std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(sorted_tuples));
            }
        }
    }
}
