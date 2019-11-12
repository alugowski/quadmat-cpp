// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#include "catch.hpp"

#include "quadmat/quadmat.h"

#include "testing_utilities.h"


TEST_CASE("Tree Nodes") {
    SECTION("index size determination") {
        SECTION("16-bit") {
            index_t size = GENERATE(1, (1u << 15u) - 1u);

            quadmat::leaf_index_type lit = quadmat::get_leaf_index_type({size, 1});
            REQUIRE(std::holds_alternative<int16_t>(lit));
        }
        SECTION("32-bit") {
            index_t size = GENERATE((1u << 15u), (1u << 31u) - 1u);

            quadmat::leaf_index_type lit = quadmat::get_leaf_index_type({1, size});
            REQUIRE(std::holds_alternative<int32_t>(lit));
        }
        SECTION("64-bit") {
            index_t size = GENERATE((1ul << 31u), (1ul << 63u) - 1u);

            quadmat::leaf_index_type lit = quadmat::get_leaf_index_type({size, 1});
            REQUIRE(std::holds_alternative<int64_t>(lit));
        }
    }
    SECTION("create leaf") {
        int size = 10;
        quadmat::identity_tuples_generator<double, int> gen(size);

        quadmat::tree_node_t<double> node = quadmat::create_leaf<double>({size, size}, size, gen);

        // make sure the indices have the expected size
        std::visit(quadmat::leaf_visitor<double>([](auto leaf, offset_t offsets, shape_t shape) {
            for (auto tup : leaf->tuples()) {
                REQUIRE(sizeof(std::get<0>(tup)) == sizeof(int16_t));
                REQUIRE(sizeof(std::get<1>(tup)) == sizeof(int16_t));
            }
        }), node);

        // dump the tuples
        vector<std::tuple<index_t, index_t, double>> tuples = dump_tuples(node);

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
        quadmat::identity_tuples_generator<double, int> gen(size);

        auto inner = std::make_shared<quadmat::inner_block<double>>(8);
        auto inner_node = quadmat::tree_node_t<double>(inner);

        // use the same block in both NW and SE positions because they are identical in an identity matrix
        quadmat::tree_node_t<double> node = quadmat::create_leaf<double>({size, size}, size, gen);
        inner->set_child(quadmat::NW, node);
        inner->set_child(quadmat::SE, node);

        // dump the tuples
        vector<std::tuple<index_t, index_t, double>> tuples = dump_tuples(inner_node);

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
