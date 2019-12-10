// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TUPLE_DUMPER_H
#define QUADMAT_TUPLE_DUMPER_H

#include "quadmat/quadmat.h"

using quadmat::index_t;
using quadmat::offset_t;
using quadmat::shape_t;
using quadmat::default_config;
using quadmat::matrix;
using quadmat::tree_node_t;
using quadmat::leaf_visitor;

/**
 * A node visitor that dumps tuples from nodes
 *
 * @tparam T
 */
template <typename T>
class tuple_dumper {
public:
    explicit tuple_dumper(vector<std::tuple<index_t, index_t, T>> &tuples, std::mutex& m) : tuples(tuples), m(m) {}

    template <typename LEAF>
    void operator()(const std::shared_ptr<LEAF>& leaf, const offset_t& offsets, const shape_t& shape) const {
        std::lock_guard lock{m};

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
    std::mutex& m;
};

/**
 * Utility to dump all tuples from a tree node and/or its children.
 * @param node
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename CONFIG = default_config>
vector<std::tuple<index_t, index_t, T>> dump_tuples(tree_node_t<T, CONFIG> node) {
    vector<std::tuple<index_t, index_t, T>> v;
    std::mutex m;
    std::visit(leaf_visitor<double, CONFIG>(tuple_dumper<T>(v, m)), node);
    return v;
}

/**
 * Utility to dump all tuples from a matrix
 * @param mat
 * @return a vector of tuples. Tuples are not sorted.
 */
template <typename T, typename CONFIG = default_config>
vector<std::tuple<index_t, index_t, T>> dump_tuples(matrix<T, CONFIG> mat) {
    return dump_tuples<T, CONFIG>(mat.get_root_bc()->get_child(0));
}

#endif //QUADMAT_TUPLE_DUMPER_H
