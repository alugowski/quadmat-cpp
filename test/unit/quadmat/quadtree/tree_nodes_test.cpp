// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#include "../../../test_dependencies/catch.hpp"

#include "quadmat/quadmat.h"

#include "../../../test_utilities/testing_utilities.h"


TEST_CASE("Tree Nodes") {
    SECTION("index size determination") {
        SECTION("16-bit") {
            Index size = GENERATE(1, (1u << 15u) - 1u);

            LeafIndex lit = GetLeafIndexType({size, 1});
            REQUIRE(std::holds_alternative<int16_t>(lit));
        }
        SECTION("32-bit") {
            Index size = GENERATE((1u << 15u), (1u << 31u) - 1u);

            LeafIndex lit = GetLeafIndexType({1, size});
            REQUIRE(std::holds_alternative<int32_t>(lit));
        }
        SECTION("64-bit") {
            Index size = GENERATE((1ul << 31u), (1ul << 63u) - 1u);

            LeafIndex lit = GetLeafIndexType({size, 1});
            REQUIRE(std::holds_alternative<int64_t>(lit));
        }
    }
    SECTION("create leaf") {
        int size = 10;
        IdentityTuplesGenerator<double, int> gen(size);

        TreeNode<double> node = CreateLeaf<double>({size, size}, size, gen);

        // make sure the indices have the expected size
        std::visit(GetLeafVisitor<double>([](auto leaf, Offset offsets, Shape shape) {
          for (auto tup : leaf->Tuples()) {
            REQUIRE(sizeof(std::get<0>(tup)) == sizeof(int16_t));
            REQUIRE(sizeof(std::get<1>(tup)) == sizeof(int16_t));
          }
        }), node);

        // dump the tuples
        std::vector<std::tuple<Index, Index, double>> tuples = DumpTuples(node);

        int count = 0;
        for (auto tup : tuples) {
            REQUIRE(std::get<0>(tup) == count);
            REQUIRE(std::get<1>(tup) == count);
            REQUIRE(std::get<2>(tup) == 1);
            count++;
        }

        REQUIRE(count == size);
    }
    SECTION("single-inner identity") {
        int size = 8;
        IdentityTuplesGenerator<double, int> gen(size);

        auto inner = std::make_shared<InnerBlock<double>>(8);
        auto inner_node = TreeNode<double>(inner);

        // use the same block in both NW and SE positions because they are identical in an identity matrix
        TreeNode<double> node = CreateLeaf<double>({size, size}, size, gen);
        inner->SetChild(NW, node);
        inner->SetChild(SE, node);

        // dump the tuples
        std::vector<std::tuple<Index, Index, double>> tuples = DumpTuples(inner_node);

        int count = 0;
        for (auto tup : tuples) {
            REQUIRE(std::get<0>(tup) == count);
            REQUIRE(std::get<1>(tup) == count);
            REQUIRE(std::get<2>(tup) == 1);
            count++;
        }

        REQUIRE(count == 2*size);
    }
}
