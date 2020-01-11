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

struct Window {
    std::string description;
    Offset offsets;
    Shape shape;
};

template <typename T, typename IT>
std::vector<std::tuple<IT, IT, T>> FilterTuples(const CannedMatrix<T, IT>& problem, const Window& window) {
    std::vector<std::tuple<IT, IT, T>> ret;

    for (auto tup : problem.sorted_tuples) {
        Index row = std::get<0>(tup);
        Index col = std::get<1>(tup);
        T value = std::get<2>(tup);

        if (row >= window.offsets.row_offset && row < (window.offsets.row_offset + window.shape.nrows) &&
            col >= window.offsets.col_offset && col < (window.offsets.col_offset + window.shape.ncols)) {
            ret.emplace_back(
                    row - window.offsets.row_offset,
                    col - window.offsets.col_offset,
                    value);
        }
    }

    return ret;
}

TEST_CASE("Window Shadow Block") {
    // get the problem
    int problem_num = GENERATE(range(0, kNumCannedMatrices));
    const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

    SECTION(problem.description) {
        auto block = DcscBlockFactory<double, Index>(problem.sorted_tuples.size(), problem.sorted_tuples).Finish();

        std::vector<Window> windows{
            Window{"full", {0, 0}, {problem.shape.nrows, problem.shape.ncols}},
            Window{"NE", {0, 0}, {problem.shape.nrows/2, problem.shape.ncols/2}},
            Window{"SE-ish", {problem.shape.nrows/2, problem.shape.ncols/2}, {problem.shape.nrows/2, problem.shape.ncols/2}},
        };

        for (const Window& window : windows) {
            auto shadow = block->GetShadowBlock(block, window.offsets, window.shape);

            // make sure tuples are correct
            auto expected_tuples = FilterTuples(problem, window);

            // check nnn
            BlockNnn nnn;
            std::visit(GetLeafVisitor<double>([&](auto leaf, Offset offsets, Shape shape) {
              nnn = leaf->GetNnn();
            }), shadow);
            REQUIRE(nnn == expected_tuples.size());

            // check size
            BlockSizeInfo size;
            std::visit(GetLeafVisitor<double>([&](auto leaf, Offset offsets, Shape shape) {
              size = leaf->GetSize();
            }), shadow);
            REQUIRE(size.GetTotalBytes() == size.overhead_bytes);

            // check tuples
            std::vector<std::tuple<Index, Index, double>> v = DumpTuples(TreeNode<double>(shadow));
            REQUIRE_THAT(v, Equals(expected_tuples));
        }
    }
}
