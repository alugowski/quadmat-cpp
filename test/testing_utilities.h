// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TESTING_UTILITIES_H
#define QUADMAT_TESTING_UTILITIES_H

#include "quadmat/quadmat.h"

using namespace quadmat;


/**
 * A node visitor that dumps tuples from nodes
 *
 * @tparam T
 */
template <typename T>
class tuple_dumper {
public:
    explicit tuple_dumper(vector<std::tuple<index_t, index_t, T>> &tuples) : tuples(tuples) {}

    template <typename LEAF>
    void operator()(quadmat::offset_t offsets, const std::shared_ptr<LEAF>& leaf) const {
        for (auto tup : leaf->tuples()) {
            tuples.emplace_back(
                    std::get<0>(tup) + offsets.row_offset,
                    std::get<1>(tup) + offsets.col_offset,
                    std::get<2>(tup)
            );
        }
    }
protected:
    vector<std::tuple<index_t, index_t, T>>& tuples;
};

/**
 * Utility to dump all tuples from a tree node and/or its children.
 * @param node
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename CONFIG = default_config>
vector<std::tuple<index_t, index_t, T>> dump_tuples(tree_node_t<T, CONFIG> node) {
    vector<std::tuple<index_t, index_t, double>> v;
    std::visit(quadmat::leaf_visitor<double>(tuple_dumper<double>(v)), node);
    return v;
}

/**
 * Utility to dump all tuples from a matrix
 * @param mat
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename CONFIG = default_config>
vector<std::tuple<index_t, index_t, T>> dump_tuples(matrix<T, CONFIG> mat) {
    return dump_tuples(mat.get_root_bc()->get_child(0));
}

/**
 * Utility function to identify whether or not a node is a leaf.
 *
 * @param node
 * @return true if node is a leaf node, false otherwise
 */
template <typename T, typename CONFIG = default_config>
bool is_leaf(tree_node_t<T, CONFIG> node) {
    return std::visit(overloaded{
            [](const std::monostate& ignored) { return false; },
            [](const std::shared_ptr<future_block<T, CONFIG>>& ignored) { return false; },
            [](const std::shared_ptr<inner_block<T, CONFIG>>& ignored) { return false; },
            [](const leaf_category_t<T, int64_t, CONFIG>& ignored) { return true; },
            [](const leaf_category_t<T, int32_t, CONFIG>& ignored) { return true; },
            [](const leaf_category_t<T, int16_t, CONFIG>& ignored) { return true; },
    }, node);
}

/**
 * Subdivide a node into an inner block and four children.
 */
template <typename T, typename CONFIG = default_config>
void subdivide_leaf(shared_ptr<block_container<T, CONFIG>> bc, int position, const shape_t& bc_shape) {
    // get the node to subdivide
    tree_node_t<T, CONFIG> node = bc->get_child(position);

    // create inner block to hold subdivided children
    auto new_inner = bc->create_inner(position);
    shape_t new_inner_shape = bc->get_child_shape(position, bc_shape);

    // use triples blocks as an intermediate data structure
    std::array<triples_block<T, index_t, CONFIG>, all_inner_positions.size()> children;

    // route tuples
    vector<std::tuple<index_t, index_t, T>> tuples = dump_tuples(node);
    offset_t offsets = new_inner->get_offsets(SE, {0, 0});
    for (auto tuple : tuples) {
        auto [row, col, value] = tuple;

        // route this tuple
        if (row < offsets.row_offset) {
            if (col < offsets.col_offset) {
                children[NW].add(row, col, value);
            } else {
                children[NE].add(row, col - offsets.col_offset, value);
            }
        } else {
            if (col < offsets.col_offset) {
                children[SW].add(row - offsets.row_offset, col, value);
            } else {
                children[SE].add(row - offsets.row_offset, col - offsets.col_offset, value);
            }
        }
    }

    // convert to dcsc and place in tree
    for (auto new_child_pos : all_inner_positions) {
        new_inner->set_child(
                new_child_pos,
                create_leaf<T, CONFIG>(
                        new_inner->get_child_shape(new_child_pos, new_inner_shape),
                        children[new_child_pos].nnn(),
                        children[new_child_pos].sorted_tuples()));
    }
}

/**
 * Matrix printer
 */
template <typename T, typename CONFIG>
std::ostream& operator<<(std::ostream& os, const matrix<T, CONFIG>& mat) {
    shape_t shape = mat.get_shape();
    // stats
    os << "matrix ( " << shape.nrows << " x " << shape.ncols << " )";

    // values
    if (shape.ncols < 40 && shape.nrows < 100) {
        quadmat::dense_string_matrix smat(shape);
        smat.fill_tuples(dump_tuples(mat));

        os << "\n" << smat.to_string();
    }
    return os;
}

/**
 * problem printer
 */
template <typename T, typename IT>
std::ostream& operator<<(std::ostream& os, const std::pair<shape_t, vector<std::tuple<IT, IT, T>>> mat) {
    shape_t shape = std::get<0>(mat);
    // stats
    os << "matrix ( " << shape.nrows << " x " << shape.ncols << " )";

    // values
    if (shape.ncols < 40 && shape.nrows < 100) {
        quadmat::dense_string_matrix smat(shape);
        smat.fill_tuples(std::get<1>(mat));

        os << "\n" << smat.to_string();
    }
    return os;
}

#endif //QUADMAT_TESTING_UTILITIES_H
