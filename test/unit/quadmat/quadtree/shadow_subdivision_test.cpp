// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto kCannedMatrices = GetCannedMatrices<double, Index>(); // NOLINT(cert-err58-cpp)
static const int kNumCannedMatrices = kCannedMatrices.size();


TEST_CASE("Shadow Subdivision") {
    // get the problem
    int problem_num = GENERATE(range(0, kNumCannedMatrices));
    const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

    SECTION(problem.description) {
        auto block = DcscBlockFactory<double, Index>(problem.sorted_tuples.size(), problem.sorted_tuples).FinishShared();
        auto leaf_node = LeafNode<double>(block);

        auto shadow_inner = shadow_subdivide<double>(leaf_node, problem.shape, GetDiscriminatingBit(problem.shape) << 1); // NOLINT(hicpp-signed-bitwise)

        SECTION("get tuples back") {
            std::vector<std::tuple<Index, Index, double>> v = DumpTuples(TreeNode<double>(shadow_inner));
            REQUIRE_THAT(v, UnorderedEquals(problem.sorted_tuples));
        }

    }
}
