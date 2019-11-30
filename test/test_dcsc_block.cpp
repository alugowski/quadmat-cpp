// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat/quadmat.h"

#include "problem_generator.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto canned_matrices =  get_canned_matrices<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("DCSC Block") {
    SECTION("basic construction") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {
            auto block = dcsc_block_factory<double, index_t>(problem.sorted_tuples.size(), problem.sorted_tuples).finish();

            SECTION("get tuples back") {
                auto sorted_range = block->tuples();
                vector<std::tuple<index_t, index_t, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.sorted_tuples));
            }
        }
    }
}
