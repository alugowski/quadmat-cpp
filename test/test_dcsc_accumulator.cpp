// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat/quadmat.h"

#include "problem_generator.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto canned_matrices =  get_canned_matrices<double, int>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("DCSC Accumulator") {

    SECTION("basic split") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, int>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {

            // split up the tuples
            int num_parts = GENERATE(range(1, 4));
            SECTION(std::to_string(num_parts) + " parts") {

                // build accumulator
                quadmat::dcsc_accumulator<double, int> accum(problem.shape);

                // slice up the tuples
                auto tuple_ranges = quadmat::slice_ranges(num_parts,
                        problem.sorted_tuples.begin(), problem.sorted_tuples.end());

                // build component blocks
                for (auto tuple_range : tuple_ranges) {
                    auto part = dcsc_block_factory<double, int>(tuple_range.size(), tuple_range).finish();

                    accum.add(part);
                }

                // collapse
                auto sum = accum.collapse();

                // get tuples back
                auto sorted_range = sum->tuples();
                vector<std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.sorted_tuples));
            }
        }
    }

    SECTION("shuffled split") {
        // get the problem
        int problem_num = GENERATE(range(1, num_canned_matrices));
        const canned_matrix<double, int>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {

            // split up the tuples
            int num_parts = GENERATE(range(2, 4));
            SECTION(std::to_string(num_parts) + " parts") {

                // build accumulator
                quadmat::dcsc_accumulator<double, int> accum(problem.shape);

                // shuffle the tuples
                vector<tuple<int, int, double>> shuffled_tuples(problem.sorted_tuples);
                quadmat::stable_shuffle(begin(shuffled_tuples), end(shuffled_tuples));
                REQUIRE_THAT(shuffled_tuples, !Equals(problem.sorted_tuples));

                // slice up the tuples
                auto tuple_ranges = quadmat::slice_ranges(num_parts,
                                                          shuffled_tuples.begin(), shuffled_tuples.end());

                // build component blocks
                for (auto tuple_range : tuple_ranges) {
                    // use a triple_block to sort these shuffled tuples
                    quadmat::triples_block<double, int> tb;
                    tb.add(tuple_range);

                    auto part = quadmat::dcsc_block_factory<double, int>(tuple_range.size(), tb.sorted_tuples()).finish();

                    accum.add(part);
                }

                // collapse
                auto sum = accum.collapse();

                // get tuples back
                auto sorted_range = sum->tuples();
                vector<std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.sorted_tuples));
            }
        }
    }

    SECTION("doubling") {
        // get the problem
        int problem_num = GENERATE(range(1, num_canned_matrices));
        const canned_matrix<double, int>& problem = canned_matrices[problem_num];

        // construct the expected tuples by doubling the value
        vector<tuple<int, int, double>> expected_tuples;
        std::transform(begin(problem.sorted_tuples), end(problem.sorted_tuples), std::back_inserter(expected_tuples),
                       [](tuple<int, int, double> tup) -> tuple<int, int, double> {
            return tuple<int, int, double>(std::get<0>(tup), std::get<1>(tup), 2*std::get<2>(tup));
        });

        SECTION(problem.description) {
            // build accumulator
            quadmat::dcsc_accumulator<double, int> accum(problem.shape);

            // build component blocks
            auto part = quadmat::dcsc_block_factory<double, int>(problem.sorted_tuples.size(), problem.sorted_tuples).finish();

            accum.add(part);
            accum.add(part);

            // collapse
            auto sum = accum.collapse();

            // get doubled tuples back
            auto sorted_range = sum->tuples();
            vector<std::tuple<int, int, double>> v(sorted_range.begin(), sorted_range.end());
            REQUIRE_THAT(v, Equals(expected_tuples));
        }
    }
}
