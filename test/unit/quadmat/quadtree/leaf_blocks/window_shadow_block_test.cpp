// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "../../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../../test_utilities/testing_utilities.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto canned_matrices = get_canned_matrices<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

struct window_t {
    string description;
    offset_t offsets;
    shape_t shape;
};

template <typename T, typename IT>
vector<std::tuple<IT, IT, T>> filter_tuples(const canned_matrix<T, IT>& problem, const window_t& window) {
    vector<std::tuple<IT, IT, T>> ret;

    for (auto tup : problem.sorted_tuples) {
        index_t row = std::get<0>(tup);
        index_t col = std::get<1>(tup);
        T value = std::get<2>(tup);

        if (row >= window.offsets.row_offset && row < (window.offsets.row_offset + window.shape.nrows) &&
            col >= window.offsets.col_offset && col < (window.offsets.col_offset + window.shape.ncols)) {
            ret.emplace_back(
                    row - window.offsets.row_offset,
                    col - window.offsets.col_offset,
                    value);
        }
    }

    return ret;
}

TEST_CASE("Window Shadow Block") {
    // get the problem
    int problem_num = GENERATE(range(0, num_canned_matrices));
    const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

    SECTION(problem.description) {
        auto block = quadmat::dcsc_block_factory<double, index_t>(problem.sorted_tuples.size(), problem.sorted_tuples).finish();

        vector<window_t> windows{
                window_t{"full", {0, 0}, {problem.shape.nrows, problem.shape.ncols}},
                window_t{"NE", {0, 0}, {problem.shape.nrows/2, problem.shape.ncols/2}},
                window_t{"SE", {problem.shape.nrows/2, problem.shape.ncols/2}, {problem.shape.nrows/2, problem.shape.ncols/2}},
        };

        for (const window_t& window : windows) {
            {//SECTION(window.description) {
                auto shadow = block->get_shadow_block(block, window.offsets, window.shape);

                // make sure tuples are correct
                auto expected_tuples = filter_tuples(problem, window);

                // check nnn
                blocknnn_t nnn;
                std::visit(leaf_visitor<double>([&](auto leaf, offset_t offsets, shape_t shape) {
                    nnn = leaf->nnn();
                }), shadow);
                REQUIRE(nnn == expected_tuples.size());

                // check size
                block_size_info size;
                std::visit(leaf_visitor<double>([&](auto leaf, offset_t offsets, shape_t shape) {
                    size = leaf->size();
                }), shadow);
                REQUIRE(size.total_bytes() == size.overhead_bytes);

                // check tuples
                vector<std::tuple<index_t, index_t, double>> v = dump_tuples(tree_node_t<double>(shadow));
                REQUIRE_THAT(v, Equals(expected_tuples));
            }
        }
    }
}
