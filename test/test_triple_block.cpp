// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat.h"

#include "problem_generator.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto canned_matrices =  get_canned_matrices<double, int>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("Triples Block") {
    SECTION("basic construction") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, int>& problem = canned_matrices[problem_num];

        // make a shuffled version
        auto shuffled_tuples(problem.sorted_tuples);
        quadmat::stable_shuffle(begin(shuffled_tuples), end(shuffled_tuples));

        SECTION(problem.description) {
            quadmat::triples_block<double, int> block(problem.shape.nrows, problem.shape.ncols);
            block.add(shuffled_tuples);

            REQUIRE(block.get_nrows() == problem.shape.nrows);
            REQUIRE(block.get_ncols() == problem.shape.ncols);

            SECTION("get tuples back") {
                vector<std::tuple<int, int, double>> v(block.begin(), block.end());
                REQUIRE_THAT(v, Equals(shuffled_tuples));
            }

            SECTION("sorted tuples") {
                auto sorted_range = block.sorted_range();
                vector<std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.sorted_tuples));
            }
        }
    }
}
