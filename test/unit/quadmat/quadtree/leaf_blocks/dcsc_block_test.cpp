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

    SECTION("GetColumn") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

        if (problem.shape.ncols < 1000) {
        SECTION(problem.description) {
                auto block_no_index = DcscBlockFactory<double, Index, ConfigNoIndex>(problem.sorted_tuples.size(),
                                                                                     problem.sorted_tuples).Finish();
                auto block_csc_index = DcscBlockFactory<double, Index, ConfigUseCscIndex>(problem.sorted_tuples.size(),
                                                                                          problem.sorted_tuples).Finish();
                auto block_bool_index =
                    DcscBlockFactory<double, Index, ConfigUseBoolMaskIndex>(problem.sorted_tuples.size(),
                                                                            problem.sorted_tuples).Finish();

                for (Index col = 0; col < problem.shape.ncols + 2; col++) {
                    auto ref_no_index = block_no_index->GetColumn(col);
                    auto ref_csc_index = block_csc_index->GetColumn(col);
                    auto ref_bool_index = block_bool_index->GetColumn(col);

                    REQUIRE(ref_no_index.col_found == ref_csc_index.col_found);
                    REQUIRE(ref_no_index.col_found == ref_bool_index.col_found);

                    if (ref_no_index.col_found) {
                        auto nrows_no_index = ref_no_index.rows_end - ref_no_index.rows_begin;
                        auto nrows_csc_index = ref_csc_index.rows_end - ref_csc_index.rows_begin;
                        auto nrows_bool_index = ref_bool_index.rows_end - ref_bool_index.rows_begin;

                        REQUIRE(nrows_no_index == nrows_csc_index);
                        REQUIRE(nrows_no_index == nrows_bool_index);
                    }
                }
            }
        }
    }
}
