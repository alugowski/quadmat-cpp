// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat/quadmat.h"

#include "problem_generator.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto canned_matrices =  get_canned_matrices<double, int>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("DCSC Block") {
    SECTION("basic construction") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, int>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {
            quadmat::dcsc_block<double, int> block(problem.sorted_tuples.size(), problem.sorted_tuples);

            SECTION("get tuples back") {
                auto sorted_range = block.tuples();
                vector<std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.sorted_tuples));
            }
        }
    }
}
