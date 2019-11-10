// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat/quadmat.h"

#include "problem_generator.h"
#include "testing_utilities.h"

using Catch::Matchers::Equals;
using Catch::Matchers::UnorderedEquals;

std::ostream& operator<<(std::ostream& os, const std::tuple<int, int, double>& tup ) {
    os << "<" << std::get<0>(tup) << ", " << std::get<1>(tup) << ", " << std::get<2>(tup) << ">";
    return os;
}

/**
 * Canned matrices
 */
static const auto multiply_problems =  get_multiply_problems<double, int>(); // NOLINT(cert-err58-cpp)
static const int num_multiply_problems = multiply_problems.size();

TEST_CASE("Multiply") {
    SECTION("DCSC Block Pair") {
        // get the problem
        int problem_num = GENERATE(range(0, num_multiply_problems));
        const multiply_problem<double, int>& problem = multiply_problems[problem_num];

        SECTION(problem.description) {
            auto a = std::make_shared<quadmat::dcsc_block<double, int>>(problem.a_shape, problem.a_sorted_tuples.size(), problem.a_sorted_tuples);
            auto b = std::make_shared<quadmat::dcsc_block<double, int>>(problem.b_shape, problem.b_sorted_tuples.size(), problem.b_sorted_tuples);

            auto result = quadmat::multiply_pair<int, int, quadmat::plus_times_semiring<double>, quadmat::sparse_spa<int, quadmat::plus_times_semiring<double>, quadmat::default_config>, quadmat::default_config>(a, b);

            REQUIRE(result->get_shape() == problem.result_shape);

            // test the result tuples
            {
                auto sorted_range = result->tuples();
                vector <std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.result_sorted_tuples));
            }
        }
    }
    SECTION("Simple tree") {
        int size = 8;
        quadmat::identity_tuples_generator<double, int> gen(size);

        auto inner = std::make_shared<quadmat::inner_block<double>>(quadmat::shape_t{2*size, 2*size}, 8);
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
            vector<std::tuple<index_t, index_t, double>> v;
            std::visit(quadmat::leaf_visitor<double>(tuple_dumper<double>(v)), inner_node);

            // construct the expected result
            quadmat::identity_tuples_generator<double, index_t> gen2(2*size);
            vector<std::tuple<index_t, index_t, double>> result_sorted_tuples(gen2.begin(), gen2.end());
            REQUIRE_THAT(v, UnorderedEquals(result_sorted_tuples));
        }
    }
}
