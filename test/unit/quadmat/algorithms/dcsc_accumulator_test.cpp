// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/problem_generator.h"

using Catch::Matchers::Equals;

/**
 * Canned matrices
 */
static const auto canned_matrices =  get_canned_matrices<double, index_t>(); // NOLINT(cert-err58-cpp)
static const int num_canned_matrices = canned_matrices.size();

TEST_CASE("DCSC Accumulator") {

    SECTION("basic split") {
        // get the problem
        int problem_num = GENERATE(range(0, num_canned_matrices));
        const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];

        SECTION(problem.description) {

            // split up the tuples
            int num_parts = GENERATE(range(1, 4));
            SECTION(std::to_string(num_parts) + " parts") {

                // build accumulator
                quadmat::dcsc_accumulator<double, index_t> accum(problem.shape);

                // slice up the tuples
                auto tuple_ranges = quadmat::slice_ranges(num_parts,
                        problem.sorted_tuples.begin(), problem.sorted_tuples.end());

                // build component blocks
                for (auto tuple_range : tuple_ranges) {
                    auto part = dcsc_block_factory<double, index_t>(tuple_range.size(), tuple_range).finish();

                    accum.add(part);
                }

                // collapse
                auto sum = accum.collapse();

                // get tuples back
                auto sorted_range = sum->tuples();
                vector<std::tuple<index_t, index_t, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem.accumulated_tuples()));
            }
        }
    }

    SECTION("shuffled split") {
        // get the problem
        int problem_num = GENERATE(range(1, num_canned_matrices));
        const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];
        auto problem_accumulated_tuples = problem.accumulated_tuples();

        SECTION(problem.description) {

            // split up the tuples
            int num_parts = GENERATE(range(2, 4));
            SECTION(std::to_string(num_parts) + " parts") {

                // build accumulator
                quadmat::dcsc_accumulator<double, index_t> accum(problem.shape);

                // shuffle the tuples
                vector<tuple<index_t, index_t, double>> shuffled_tuples(problem_accumulated_tuples);
                quadmat::stable_shuffle(begin(shuffled_tuples), end(shuffled_tuples));
                REQUIRE_THAT(shuffled_tuples, !Equals(problem_accumulated_tuples));

                // slice up the tuples
                auto tuple_ranges = quadmat::slice_ranges(num_parts,
                                                          shuffled_tuples.begin(), shuffled_tuples.end());

                // build component blocks
                for (auto tuple_range : tuple_ranges) {
                    // use a triple_block to sort these shuffled tuples
                    quadmat::triples_block<double, index_t> tb;
                    tb.add(tuple_range);

                    auto part = quadmat::dcsc_block_factory<double, index_t>(tuple_range.size(), tb.sorted_tuples()).finish();

                    accum.add(part);
                }

                // collapse
                auto sum = accum.collapse();

                // get tuples back
                auto sorted_range = sum->tuples();
                vector<std::tuple<index_t, index_t, double>> v(sorted_range.begin(), sorted_range.end());
                REQUIRE_THAT(v, Equals(problem_accumulated_tuples));
            }
        }
    }

    SECTION("doubling") {
        // get the problem
        int problem_num = GENERATE(range(1, num_canned_matrices));
        const canned_matrix<double, index_t>& problem = canned_matrices[problem_num];
        auto problem_accumulated_tuples = problem.accumulated_tuples();

        // construct the expected tuples by doubling the value
        vector<tuple<index_t, index_t, double>> expected_tuples;
        std::transform(begin(problem_accumulated_tuples), end(problem_accumulated_tuples), std::back_inserter(expected_tuples),
                       [](tuple<index_t, index_t, double> tup) -> tuple<index_t, index_t, double> {
            return tuple<index_t, index_t, double>(std::get<0>(tup), std::get<1>(tup), 2*std::get<2>(tup));
        });

        SECTION(problem.description) {
            // build accumulator
            quadmat::dcsc_accumulator<double, index_t> accum(problem.shape);

            // build component blocks
            auto part = quadmat::dcsc_block_factory<double, index_t>(problem_accumulated_tuples.size(), problem_accumulated_tuples).finish();

            accum.add(part);
            accum.add(part);

            // collapse
            auto sum = accum.collapse();

            // get doubled tuples back
            auto sorted_range = sum->tuples();
            vector<std::tuple<index_t, index_t, double>> v(sorted_range.begin(), sorted_range.end());
            REQUIRE_THAT(v, Equals(expected_tuples));
        }
    }
}
