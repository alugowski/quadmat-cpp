// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS

#include "../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../test_utilities/testing_utilities.h"

/**
 * Read problems
 */
static const auto kMultiplyProblems = GetFsMultiplyProblems("medium"); // NOLINT(cert-err58-cpp)
static const int kNumMultiplyProblems = kMultiplyProblems.size();

TEST_CASE("Multiply - Medium Tests") {

    SECTION("DefaultConfig") {

        // get the problem
        int problem_num = GENERATE(range(0, kNumMultiplyProblems));
        const auto& problem = kMultiplyProblems[problem_num];

        bool is_triple_product = !problem.c_path.empty();

        SECTION(problem.description) {

            std::ifstream a_stream{problem.a_path};
            auto a = MatrixMarket::Load(a_stream);

            std::ifstream b_stream{problem.b_path};
            auto b = MatrixMarket::Load(b_stream);

            REQUIRE("" == SanityCheck(a)); // NOLINT(readability-container-size-empty)
            REQUIRE("" == SanityCheck(b)); // NOLINT(readability-container-size-empty)

            if (is_triple_product) {
                std::ifstream c_stream{problem.c_path};
                auto c = MatrixMarket::Load(c_stream);

                std::ifstream product_abc_stream{problem.product_abc_path};
                auto expected_result = MatrixMarket::Load(product_abc_stream);

                // multiply
                auto result_ab = Multiply<PlusTimesSemiring<double>>(a, b);
                auto result = Multiply<PlusTimesSemiring<double>>(result_ab, c);

                REQUIRE("" == SanityCheck(result)); // NOLINT(readability-container-size-empty)

                // test the result
                REQUIRE_THAT(result, MatrixEquals(expected_result));
            } else {
                std::ifstream product_ab_stream{problem.product_ab_path};
                auto expected_result = MatrixMarket::Load(product_ab_stream);

                // multiply
                auto result = Multiply<PlusTimesSemiring<double>>(a, b);

                REQUIRE("" == SanityCheck(result)); // NOLINT(readability-container-size-empty)

                // test the result
                REQUIRE_THAT(result, MatrixEquals(expected_result));
            }
        }
    }
}
