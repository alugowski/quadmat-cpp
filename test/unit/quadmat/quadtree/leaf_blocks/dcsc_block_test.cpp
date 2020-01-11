// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../../test_utilities/testing_utilities.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto kCannedMatrices = GetCannedMatrices<double, Index>(); // NOLINT(cert-err58-cpp)
static const int kNumCannedMatrices = kCannedMatrices.size();

TEST_CASE("DCSC Block") {
    SECTION("basic construction") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

        SECTION(problem.description) {
            auto block = DcscBlockFactory<double, Index>(problem.sorted_tuples.size(), problem.sorted_tuples).Finish();

            SECTION("get tuples back") {
                auto sorted_range = block->Tuples();
                std::vector<std::tuple<Index, Index, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.sorted_tuples));
            }
        }
    }
}
