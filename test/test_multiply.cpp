// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat.h"

#include "problem_generator.h"

using Catch::Matchers::Equals;

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

            auto result = quadmat::multiply_pair<int, int, quadmat::plus_times_semiring<double>, quadmat::sparse_spa<int, quadmat::plus_times_semiring<double>, quadmat::basic_settings>, quadmat::basic_settings>(a, b);

            REQUIRE(result->get_shape() == problem.result_shape);

            // test the result tuples
            {
                auto sorted_range = result->tuples();
                vector <std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.result_sorted_tuples));
            }
        }
    }
}
