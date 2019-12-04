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

    SECTION("Simple Trees") {
        auto sub_type = GENERATE(
                subdivision_t{false, false, "leaf * leaf"},
                subdivision_t{true, false, "single inner * leaf"},
                subdivision_t{false, true, "leaf * single inner"},
                subdivision_t{true, true, "single inner * single inner"}
        );

        SECTION(sub_type.description) {
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

    SECTION("leaf_split_threshold=4") {
        auto sub_type = GENERATE(
                // leaf * leaf already handled in another test case
                subdivision_t{true, false, "tree * leaf"},
                subdivision_t{false, true, "leaf * tree"},
                subdivision_t{true, true, "tree * tree"}
        );

        SECTION(sub_type.description) {
            // get the problem
            int problem_num = GENERATE(range(0, num_multiply_problems));
            const multiply_problem<double, index_t> &problem = multiply_problems[problem_num];

            SECTION(problem.description) {
                matrix<double, config_split_4> a{problem.a.shape}, b{problem.b.shape};

                // construct matrix a as either a tree or a single leaf
                if (sub_type.subdivide_left) {
                    a = matrix_from_tuples<double, config_split_4>(problem.a.shape,
                                                                   problem.a.sorted_tuples.size(),
                                                                   problem.a.sorted_tuples);
                } else {
                    a = single_leaf_matrix_from_tuples<double, config_split_4>(problem.a.shape,
                                                                               problem.a.sorted_tuples.size(),
                                                                               problem.a.sorted_tuples);
                }

                // construct matrix b as either a tree or a single leaf
                if (sub_type.subdivide_right) {
                    b = matrix_from_tuples<double, config_split_4>(problem.b.shape,
                                                                   problem.b.sorted_tuples.size(),
                                                                   problem.b.sorted_tuples);
                } else {
                    b = single_leaf_matrix_from_tuples<double, config_split_4>(problem.b.shape,
                                                                               problem.b.sorted_tuples.size(),
                                                                               problem.b.sorted_tuples);
                }
                REQUIRE("" == sanity_check(a)); // NOLINT(readability-container-size-empty)
                REQUIRE("" == sanity_check(b)); // NOLINT(readability-container-size-empty)

                // multiply
                auto result = multiply<plus_times_semiring<double>>(a, b);

                REQUIRE("" == sanity_check(result)); // NOLINT(readability-container-size-empty)

                // test the result
                REQUIRE_THAT(result, MatrixEquals(problem.result, config_split_4()));
            }
        }
    }
}
