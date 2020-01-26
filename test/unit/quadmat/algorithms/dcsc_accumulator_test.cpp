// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto kCannedMatrices = GetCannedMatrices<double, Index>(); // NOLINT(cert-err58-cpp)
static const int kNumCannedMatrices = kCannedMatrices.size();

TEST_CASE("DCSC Accumulator") {

    SECTION("basic split") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

        SECTION(problem.description) {

            // split up the tuples
            int num_parts = GENERATE(range(1, 4));
            SECTION(std::to_string(num_parts) + " parts") {

                // build accumulator
                DcscAccumulator<double, Index> accum(problem.shape);

                // slice up the tuples
                auto tuple_ranges = SliceRanges(num_parts, problem.sorted_tuples.begin(), problem.sorted_tuples.end());

                // build component blocks
                for (auto tuple_range : tuple_ranges) {
                    auto part = DcscBlockFactory<double, Index>(tuple_range.size(), tuple_range).Finish();

                    accum.Add(part);
                }

                // collapse
                auto sum = accum.Collapse();

                // matrix comparison
                auto expected_tuples = problem.GetAccumulatedTuples();

                if (num_parts == 1 && problem.description.find("every entry duplicated") != std::string::npos) {
                    // accumulator will just return the single child, so the duplicate tuples should remain dupes.
                    expected_tuples = problem.sorted_tuples;
                }

                REQUIRE_THAT(Matrix<double>(problem.shape, sum),MatrixEquals<double>(problem.shape, expected_tuples));

                // tuple comparison
                auto sorted_range = sum->Tuples();
                std::vector<std::tuple<Index, Index, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(expected_tuples));
            }
        }
    }

    SECTION("shuffled split") {
        // get the problem
        int problem_num = GENERATE(range(1, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];
        auto problem_accumulated_tuples = problem.GetAccumulatedTuples();

        SECTION(problem.description) {

            // split up the tuples
            int num_parts = GENERATE(range(2, 4));
            SECTION(std::to_string(num_parts) + " parts") {

                // build accumulator
                DcscAccumulator<double, Index> accum(problem.shape);

                // shuffle the tuples
                std::vector<std::tuple<Index, Index, double>> shuffled_tuples(problem_accumulated_tuples);
                StableShuffle(begin(shuffled_tuples), end(shuffled_tuples));
                REQUIRE_THAT(shuffled_tuples, !Equals(problem_accumulated_tuples));

                // slice up the tuples
                auto tuple_ranges = SliceRanges(num_parts, shuffled_tuples.begin(), shuffled_tuples.end());

                // build component blocks
                for (auto tuple_range : tuple_ranges) {
                    // use a triple_block to sort these shuffled tuples
                    TriplesBlock<double, Index> tb;
                    tb.Add(tuple_range);

                    auto part = DcscBlockFactory<double, Index>(tuple_range.size(), tb.SortedTuples()).Finish();

                  accum.Add(part);
                }

                // collapse
                auto sum = accum.Collapse();

                // matrix comparison
                REQUIRE_THAT(Matrix<double>(problem.shape, sum), MatrixEquals<double>(problem.shape, problem_accumulated_tuples));

                // tuple comparison
                auto sorted_range = sum->Tuples();
                std::vector<std::tuple<Index, Index, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem_accumulated_tuples));
            }
        }
    }

    SECTION("doubling") {
        // get the problem
        int problem_num = GENERATE(range(1, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];
        auto problem_accumulated_tuples = problem.GetAccumulatedTuples();

        // construct the expected tuples by doubling the value
        std::vector<std::tuple<Index, Index, double>> expected_tuples;
        std::transform(begin(problem_accumulated_tuples),
                       end(problem_accumulated_tuples),
                       std::back_inserter(expected_tuples),
                       [](std::tuple<Index, Index, double> tup) -> std::tuple<Index, Index, double> {
                           return std::tuple<Index, Index, double>(std::get<0>(tup),
                                                                   std::get<1>(tup),
                                                                   2 * std::get<2>(tup));
                       });

        SECTION(problem.description) {
            // build accumulator
            DcscAccumulator<double, Index> accum(problem.shape);

            // build component blocks
            auto part = DcscBlockFactory<double, Index>(problem_accumulated_tuples.size(),
                                                        problem_accumulated_tuples).Finish();

            accum.Add(part);
            accum.Add(part);

            // collapse
            auto sum = accum.Collapse();

            // matrix comparison
            REQUIRE_THAT(Matrix<double>(problem.shape, sum), MatrixEquals<double>(problem.shape, expected_tuples));

            // tuple comparison
            auto sorted_range = sum->Tuples();
            std::vector<std::tuple<Index, Index, double>> v(sorted_range.begin(), sorted_range.end());
            REQUIRE_THAT(v, Equals(expected_tuples));
        }
    }
}
