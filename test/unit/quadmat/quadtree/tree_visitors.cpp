// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

/**
 * Canned matrices
 */
static const auto canned_matrices =  get_canned_matrices<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();


TEST_CASE("Tree Visitors") {

    SECTION("leaf size") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {
            auto mat = matrix_from_tuples<double, config_split_4>(problem.shape,
                                                                  problem.sorted_tuples.size(),
                                                                  problem.sorted_tuples);

            auto node = mat.get_root_bc()->get_child(0);

            block_size_info sizes;
            std::visit(leaf_visitor<double, config_split_4>([&](auto leaf, offset_t offsets, shape_t shape) {
                sizes = leaf->size() + sizes;
            }), node);

            REQUIRE(sizes.nnn == problem.sorted_tuples.size());
            REQUIRE(sizes.value_bytes == sizes.nnn * sizeof(double));
            REQUIRE(sizes.total_bytes() >= sizes.nnn * sizeof(double));
        }
    }

    SECTION("future blocks") {
        auto future_node = tree_node_t<double>(std::make_shared<future_block<double>>());

        bool visited = false;
        std::visit(leaf_visitor<double>([&](auto leaf, offset_t offsets, shape_t shape) {
            visited = true;
        }), future_node);

        REQUIRE(!visited);
    }
}
