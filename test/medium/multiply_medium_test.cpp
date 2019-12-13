// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS

#include "../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../test_utilities/testing_utilities.h"

/**
 * Read problems
 */
static const auto multiply_problems =  get_fs_multiply_problems("medium"); // NOLINT(cert-err58-cpp)
static const int num_multiply_problems = multiply_problems.size();

TEST_CASE("Multiply") {

    SECTION("default_config") {

        // get the problem
        int problem_num = GENERATE(range(0, num_multiply_problems));
        const auto& problem = multiply_problems[problem_num];

        bool is_triple_product = !problem.c_path.empty();

        SECTION(problem.description) {

            std::ifstream a_stream{problem.a_path};
            auto a = matrix_market::load(a_stream);

            std::ifstream b_stream{problem.b_path};
            auto b = matrix_market::load(b_stream);

            REQUIRE("" == sanity_check(a)); // NOLINT(readability-container-size-empty)
            REQUIRE("" == sanity_check(b)); // NOLINT(readability-container-size-empty)

            if (is_triple_product) {
                std::ifstream c_stream{problem.c_path};
                auto c = matrix_market::load(c_stream);

                std::ifstream product_abc_stream{problem.product_abc_path};
                auto expected_result = matrix_market::load(product_abc_stream);

                // multiply
                auto result_ab = multiply<plus_times_semiring<double>>(a, b);
                auto result = multiply<plus_times_semiring<double>>(result_ab, c);

                REQUIRE("" == sanity_check(result)); // NOLINT(readability-container-size-empty)

                // test the result
                REQUIRE_THAT(result, MatrixEquals(expected_result));
            } else {
                std::ifstream product_ab_stream{problem.product_ab_path};
                auto expected_result = matrix_market::load(product_ab_stream);

                // multiply
                auto result = multiply<plus_times_semiring<double>>(a, b);

                REQUIRE("" == sanity_check(result)); // NOLINT(readability-container-size-empty)

                // test the result
                REQUIRE_THAT(result, MatrixEquals(expected_result));
            }
        }
    }
}
