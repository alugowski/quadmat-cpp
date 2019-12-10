// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

using Catch::Matchers::Equals;
using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto canned_matrices =  get_canned_matrices<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("Tree Construction") {
    SECTION("create_leaf") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {
            auto leaf = create_leaf<double>(problem.shape, problem.sorted_tuples.size(), problem.sorted_tuples);

            vector<std::tuple<index_t, index_t, double>> v = dump_tuples(tree_node_t<double>(leaf));
            REQUIRE_THAT(v, Equals(problem.sorted_tuples));
        }
    }
    SECTION("subdivide") {
        SECTION("leaf_split_threshold=4") {
            // get the problem
            int problem_num = GENERATE(range(0, num_canned_matrices));
            const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

            SECTION(problem.description) {
                auto triples = std::make_shared<triples_block<double, index_t, config_split_4>>();
                triples->add(problem.sorted_tuples);

                auto subdivided_node = subdivide(triples, problem.shape);

                // ensure the tuples that come back are correct
                vector<std::tuple<index_t, index_t, double>> v = dump_tuples<double, config_split_4>(tree_node_t<double, config_split_4>(subdivided_node));
                REQUIRE_THAT(v, UnorderedEquals(problem.sorted_tuples));

                // ensure that leaf sizes are as expected
                std::visit(leaf_visitor<double, config_split_4>([&](auto leaf, offset_t offsets, shape_t shape) {
                    REQUIRE(leaf->nnn() > 0); // empty blocks should be created as std::monospace
                    REQUIRE(leaf->nnn() <= config_split_4::leaf_split_threshold);
                }), subdivided_node);
            }
        }
    }
}
