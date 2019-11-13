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

TEST_CASE("Multiply") {
    SECTION("DCSC Block Pair") {
        // get the problem
        int problem_num = GENERATE(range(0, num_multiply_problems));
        const multiply_problem<double, index_t>& problem = multiply_problems[problem_num];

        SECTION(problem.description) {
            auto a = std::make_shared<quadmat::dcsc_block<double, index_t>>(problem.a.sorted_tuples.size(), problem.a.sorted_tuples);
            auto b = std::make_shared<quadmat::dcsc_block<double, index_t>>(problem.b.sorted_tuples.size(), problem.b.sorted_tuples);

            shape_t result_shape = {
                    .nrows = problem.a.shape.nrows,
                    .ncols = problem.b.shape.ncols
            };

            auto result = quadmat::multiply_pair<index_t, index_t, quadmat::plus_times_semiring<double>, quadmat::sparse_spa<index_t, quadmat::plus_times_semiring<double>, quadmat::default_config>, quadmat::default_config>(a, b, result_shape);

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
            auto a = matrix_from_tuples<double>(problem.a.shape, problem.a.sorted_tuples.size(), problem.a.sorted_tuples);
            auto b = matrix_from_tuples<double>(problem.b.shape, problem.b.sorted_tuples.size(), problem.b.sorted_tuples);

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
    SECTION("Simple tree") {
        int size = 8;
        quadmat::identity_tuples_generator<double, index_t> gen(size);

        auto inner = std::make_shared<quadmat::inner_block<double>>(8);
        auto inner_node = quadmat::tree_node_t<double>(inner);

        // use the same block in both NW and SE positions because they are identical in an identity matrix
        quadmat::tree_node_t<double> node = quadmat::create_leaf<double>({size, size}, size, gen);
        inner->set_child(quadmat::NW, node);
        inner->set_child(quadmat::SE, node);

        // create the result
        auto sbc = std::make_shared<quadmat::single_block_container<double>>(quadmat::shape_t{2*size, 2*size});

        // multiply
        quadmat::spawn_multiply_job<quadmat::plus_times_semiring<double>> job(quadmat::pair_set_t<double, double, quadmat::default_config>{inner_node, inner_node}, sbc, 0, {0, 0}, quadmat::shape_t{2 * size, 2 * size});
        job.run();

        // test the result
        {
            vector<std::tuple<index_t, index_t, double>> v = dump_tuples(inner_node);

            // construct the expected result
            quadmat::identity_tuples_generator<double, index_t> gen2(2*size);
            vector<std::tuple<index_t, index_t, double>> result_sorted_tuples(gen2.begin(), gen2.end());
            REQUIRE_THAT(v, UnorderedEquals(result_sorted_tuples));
        }
    }
}
