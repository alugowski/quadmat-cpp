// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

/**
 * Canned matrices
 */
static const auto kCannedMatrices = GetCannedMatrices<double, Index>(); // NOLINT(cert-err58-cpp)
static const int kNumCannedMatrices = kCannedMatrices.size();

TEST_CASE("Tree Visitors") {

    SECTION("leaf size") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

        SECTION(problem.description) {
            auto mat = MatrixFromTuples<double, ConfigSplit4>(problem.shape,
                                                              problem.sorted_tuples.size(),
                                                              problem.sorted_tuples);

            auto node = mat.GetRootBC()->GetChild(0);

            BlockSizeInfo sizes;
            std::visit(GetLeafVisitor<double, ConfigSplit4>([&](auto leaf, Offset offsets, Shape shape) {
                sizes = leaf->GetSize() + sizes;
            }), node);

            REQUIRE(sizes.nnn == problem.sorted_tuples.size());
            REQUIRE(sizes.value_bytes == sizes.nnn * sizeof(double));
            REQUIRE(sizes.GetTotalBytes() >= sizes.nnn * sizeof(double));
        }
    }

    SECTION("future blocks") {
        auto future_node = TreeNode<double>(std::make_shared<FutureBlock<double>>());

        bool visited = false;
        std::visit(GetLeafVisitor<double>([&](auto leaf, Offset offsets, Shape shape) {
            visited = true;
        }), future_node);

        REQUIRE(!visited);
    }
}
