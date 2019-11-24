// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat/quadmat.h"

#include "testing_utilities.h"

using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto canned_matrices = get_canned_matrices<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();


TEST_CASE("Shadow Subdivision") {
    // get the problem
    int problem_num = GENERATE(range(0, num_canned_matrices));
    const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

    SECTION(problem.description) {
        auto block = std::make_shared<quadmat::dcsc_block<double, index_t>>(problem.sorted_tuples.size(), problem.sorted_tuples);
        auto node = tree_node_t<double>(block);

        auto shadow_inner = shadow_subdivide(node, problem.shape);

        SECTION("get tuples back") {
            vector<std::tuple<index_t, index_t, double>> v = dump_tuples(tree_node_t<double>(shadow_inner));
            REQUIRE_THAT(v, UnorderedEquals(problem.sorted_tuples));
        }

    }
}
