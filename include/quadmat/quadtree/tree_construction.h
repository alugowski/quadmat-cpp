// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TREE_CONSTRUCTION_H
#define QUADMAT_TREE_CONSTRUCTION_H

#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/quadtree/leaf_blocks/dcsc_block.h"

namespace quadmat {
    /**
     * Create a leaf block of the desired shape and tuples.
     *
     * The leaf type, including index size, is determined here.
     *
     * @tparam T data type
     * @tparam CONFIG settings
     * @tparam GEN tuple generator. Must have a begin() and end() that return tuple<IT, IT, T> for some integer type IT
     * @param shape shape of leaf
     * @param nnn estimated number of non-nulls, i.e. col_ordered_gen.size().
     * @param col_ordered_gen tuple generator
     * @return
     */
    template <typename T, typename CONFIG = default_config, typename GEN>
    leaf_node_t<T, CONFIG> create_leaf(const shape_t& shape, const blocknnn_t nnn, const GEN col_ordered_gen) {
        leaf_index_type desired_index_type = get_leaf_index_type(shape);

        return std::visit(overloaded{
                [&](int64_t dim) -> leaf_node_t<T, CONFIG> { return leaf_category_t<T, int64_t, CONFIG>(dcsc_block_factory<T, int64_t, CONFIG>(nnn, col_ordered_gen).finish()); },
                [&](int32_t dim) -> leaf_node_t<T, CONFIG> { return leaf_category_t<T, int32_t, CONFIG>(dcsc_block_factory<T, int32_t, CONFIG>(nnn, col_ordered_gen).finish()); },
                [&](int16_t dim) -> leaf_node_t<T, CONFIG> { return leaf_category_t<T, int16_t, CONFIG>(dcsc_block_factory<T, int16_t, CONFIG>(nnn, col_ordered_gen).finish()); },
        }, desired_index_type);
    }


}

#endif //QUADMAT_TREE_CONSTRUCTION_H
