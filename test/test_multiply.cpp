// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS

#include "catch.hpp"

#include "quadmat/quadmat.h"

#include "problem_generator.h"
#include "testing_utilities.h"

using Catch::Matchers::Equals;
using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto multiply_problems =  get_multiply_problems<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_multiply_problems = multiply_problems.size();

struct subdivision_t {
    bool subdivide_left;
    bool subdivide_right;
    string description;
};

TEST_CASE("Multiply") {
    SECTION("DCSC Block Pair") {
        // get the problem
        int problem_num = GENERATE(range(0, num_multiply_problems));
        const multiply_problem<double, index_t>& problem = multiply_problems[problem_num];

        SECTION(problem.description) {
            auto a = quadmat::dcsc_block_factory<double, index_t>(problem.a.sorted_tuples.size(), problem.a.sorted_tuples).finish();
            auto b = quadmat::dcsc_block_factory<double, index_t>(problem.b.sorted_tuples.size(), problem.b.sorted_tuples).finish();

            shape_t result_shape = {
                    .nrows = problem.a.shape.nrows,
                    .ncols = problem.b.shape.ncols
            };

            auto result = quadmat::multiply_pair<
                    quadmat::dcsc_block<double, index_t>,
                    quadmat::dcsc_block<double, index_t>,
                    index_t,
                    quadmat::plus_times_semiring<double>,
                    quadmat::sparse_spa<index_t, quadmat::plus_times_semiring<double>, quadmat::default_config>,
                    quadmat::default_config>(a, b, result_shape);

            // test the result tuples
            {
                auto sorted_range = result->tuples();
                vector <std::tuple<index_t, index_t, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(matrix(problem.result.shape, tree_node_t<double>(result)), MatrixEquals(problem.result));
            }
        }
    }
    SECTION("Single inner_block * Single inner_block") {
        // get the problem
        int problem_num = GENERATE(range(0, num_multiply_problems));
        const multiply_problem<double, index_t>& problem = multiply_problems[problem_num];

        SECTION(problem.description) {
            auto a = single_leaf_matrix_from_tuples<double>(problem.a.shape, problem.a.sorted_tuples.size(), problem.a.sorted_tuples);
            auto b = single_leaf_matrix_from_tuples<double>(problem.b.shape, problem.b.sorted_tuples.size(), problem.b.sorted_tuples);

            // make sure the matrices look how this test assumes they do
            REQUIRE(is_leaf(a.get_root_bc()->get_child(0)));
            REQUIRE(is_leaf(b.get_root_bc()->get_child(0)));

            // subdivide
            subdivide_leaf(a.get_root_bc(), 0, a.get_shape());
            subdivide_leaf(b.get_root_bc(), 0, b.get_shape());

            // multiply
            auto result = multiply<plus_times_semiring<double>>(a, b);

            // test the result
            REQUIRE_THAT(result, MatrixEquals(problem.result));
        }
    }
    SECTION("Simple Trees") {
        // get the problem
        int problem_num = GENERATE(range(0, num_multiply_problems));
        const multiply_problem<double, index_t>& problem = multiply_problems[problem_num];

        SECTION(problem.description) {
            auto a = single_leaf_matrix_from_tuples<double>(problem.a.shape, problem.a.sorted_tuples.size(), problem.a.sorted_tuples);
            auto b = single_leaf_matrix_from_tuples<double>(problem.b.shape, problem.b.sorted_tuples.size(), problem.b.sorted_tuples);

            // make sure the matrices look how this test assumes they do
            REQUIRE(is_leaf(a.get_root_bc()->get_child(0)));
            REQUIRE(is_leaf(b.get_root_bc()->get_child(0)));

            auto sub_type = GENERATE(
                    subdivision_t{false, false, "leaf * leaf"},
                    subdivision_t{true, false, "single inner * leaf"},
                    subdivision_t{false, true, "leaf * single inner"},
                    subdivision_t{true, true, "single inner * single inner"}
                    );

            SECTION(sub_type.description) {
                // subdivide
                if (sub_type.subdivide_left) {
                    subdivide_leaf(a.get_root_bc(), 0, a.get_shape());
                }
                if (sub_type.subdivide_right) {
                    subdivide_leaf(b.get_root_bc(), 0, b.get_shape());
                }

                // multiply
                auto result = multiply<plus_times_semiring<double>>(a, b);

                // test the result
                REQUIRE_THAT(result, MatrixEquals(problem.result));
            }
        }
    }
}
