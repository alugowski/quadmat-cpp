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

TEST_CASE("Triples Block") {
    SECTION("basic construction") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

        // make a shuffled version
        auto shuffled_tuples(problem.sorted_tuples);
        StableShuffle(begin(shuffled_tuples), end(shuffled_tuples));

        SECTION(problem.description) {
            TriplesBlock<double, Index> block;
            block.Add(shuffled_tuples);

            SECTION("get tuples back") {
                auto original_tuples = block.OriginalTuples();
                std::vector<std::tuple<Index, Index, double>> v(original_tuples.begin(), original_tuples.end());
                REQUIRE_THAT(v, Equals(shuffled_tuples));
            }

            SECTION("sorted tuples") {
                auto sorted_tuples = block.SortedTuples();
                std::vector<std::tuple<Index, Index, double>> v(sorted_tuples.begin(), sorted_tuples.end());
                REQUIRE_THAT(v, Equals(problem.sorted_tuples));
            }
        }
    }
}
