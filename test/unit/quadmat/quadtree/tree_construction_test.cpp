// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"

using Catch::Matchers::Equals;
using Catch::Matchers::UnorderedEquals;

/**
 * Canned matrices
 */
static const auto kCannedMatrices = GetCannedMatrices<double, Index>(); // NOLINT(cert-err58-cpp)
static const int kNumCannedMatrices = kCannedMatrices.size();

TEST_CASE("Tree Construction") {
    SECTION("CreateLeaf") {
        // get the problem
        int problem_num = GENERATE(range(0, kNumCannedMatrices));
        const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

        SECTION(problem.description) {
            auto leaf = CreateLeaf<double>(problem.shape, problem.sorted_tuples.size(), problem.sorted_tuples);

            std::vector<std::tuple<Index, Index, double>> v = DumpTuples(TreeNode<double>(leaf));
            REQUIRE_THAT(v, Equals(problem.sorted_tuples));
        }
    }
    SECTION("Subdivide") {
        SECTION("LeafSplitThreshold=4") {
            // get the problem
            int problem_num = GENERATE(range(0, kNumCannedMatrices));
            const CannedMatrix<double, Index>& problem = kCannedMatrices[problem_num];

            SECTION(problem.description) {
                auto triples = std::make_shared<TriplesBlock<double, Index, ConfigSplit4>>();
                triples->Add(problem.sorted_tuples);

                auto subdivided_node = Subdivide(triples, problem.shape);

                // ensure the tuples that come back are correct
                std::vector<std::tuple<Index, Index, double>> v =
                    DumpTuples<double, ConfigSplit4>(TreeNode<double, ConfigSplit4>(subdivided_node));
                REQUIRE_THAT(v, UnorderedEquals(problem.sorted_tuples));

                // ensure that leaf sizes are as expected
                std::visit(GetLeafVisitor<double, ConfigSplit4>([&](auto leaf, Offset offsets, Shape shape) {
                    REQUIRE(leaf->GetNnn() > 0); // empty blocks should be created as std::monospace
                    REQUIRE(leaf->GetNnn() <= ConfigSplit4::LeafSplitThreshold);
                }), subdivided_node);
            }
        }
    }
}
