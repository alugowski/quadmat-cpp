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

TEST_CASE("Parallel Tree Destructor") {
    auto p = GENERATE(1, 2, 4, 8);

    SECTION(std::string("p=") + std::to_string(p)) {
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index> &problem = kCannedMatrices[problem_num];

        SECTION(problem.description) {
            auto matrix = MatrixFromTuples<double, ConfigSplit4>(problem.shape,
                                                       problem.sorted_tuples.size(),
                                                       problem.sorted_tuples);

            matrix.ParallelDestroy(p);
        }
    }

    SECTION("invalid") {
        std::shared_ptr<InnerBlock<double, DefaultConfig>> empty;
        ParallelTreeDestructor<double, DefaultConfig>::Destroy(empty, 1);
    }
}
