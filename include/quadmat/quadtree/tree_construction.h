// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TREE_CONSTRUCTION_H
#define QUADMAT_TREE_CONSTRUCTION_H

#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/quadtree/leaf_blocks/dcsc_block.h"
#include "quadmat/quadtree/leaf_blocks/triples_block.h"
#include "quadmat/util/base_iterators.h"

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

    /**
     * Implementation of triples block subdivision.
     */
    template <typename T, typename IT, typename CONFIG>
    tree_node_t<T, CONFIG> subdivide_impl(const shared_ptr<triples_block<T, IT, CONFIG>>& block,
                                          const offset_t offsets, const shape_t& shape,
                                          shared_ptr<typename triples_block<T, IT, CONFIG>::permutation_vector_type> permutation,
                                          typename triples_block<T, IT, CONFIG>::permutation_vector_type::iterator perm_begin,
                                          typename triples_block<T, IT, CONFIG>::permutation_vector_type::iterator perm_end) {
        // if there are no triples then return an empty node
        if ((perm_end - perm_begin) == 0) {
            return tree_node_t<T, CONFIG>();
        }

        // if there are only one leaf's worth of triples then construct a single leaf
        if ((perm_end - perm_begin) < CONFIG::leaf_split_threshold) {
            // sort
            std::sort(perm_begin, perm_end,
                      [&](size_t i, size_t j) {
                          if (block->get_col(i) != block->get_col(j)) {
                              return block->get_col(i) < block->get_col(j);
                          } else if (block->get_row(i) != block->get_row(j)) {
                              return block->get_row(i) < block->get_row(j);
                          } else {
                              return i < j;
                          }
                      });

            return create_leaf<T, CONFIG>(
                    shape,
                    (perm_end - perm_begin),
                    offset_tuples_neg(block->permuted_tuples(permutation, perm_begin, perm_end), offsets));
        }

        // subdivision is required
        const index_t discriminating_bit = get_discriminating_bit(shape);
        auto ret = std::make_shared<inner_block<T, CONFIG>>(discriminating_bit);

        // partition east and west children by column
        std::partition(perm_begin, perm_end,
                       [&](size_t i) -> bool {
                           return (block->get_col(i) - offsets.col_offset < discriminating_bit);
                       });

        // find the east-west divide
        auto ew_i = std::lower_bound(
                perm_begin,
                perm_end,
                discriminating_bit,
                [&](size_t i, index_t value) -> bool {
                    return block->get_col(i) - offsets.col_offset < value;
                });

        // partition the north-south children.
        // it's the same operation for both east and west
        std::array chunks {
                std::make_tuple(NW, SW, perm_begin, ew_i),
                std::make_tuple(NE, SE, ew_i, perm_end)
        };

        for (auto [north_child, south_child, chunk_begin, chunk_end] : chunks) {
            // partition the north and south children by row
            std::partition(chunk_begin, chunk_end,
                           [&](size_t i) -> bool {
                               return (block->get_row(i) - offsets.row_offset < discriminating_bit);
                           });

            // find the north-south divide
            auto ns_i = std::lower_bound(
                    chunk_begin,
                    chunk_end,
                    discriminating_bit,
                    [&](size_t i, index_t value) -> bool {
                        return block->get_row(i) - offsets.row_offset < value;
                    });

            ret->set_child(north_child, subdivide_impl(block, ret->get_offsets(north_child, offsets), ret->get_child_shape(north_child, shape), permutation, chunk_begin, ns_i));
            ret->set_child(south_child, subdivide_impl(block, ret->get_offsets(south_child, offsets), ret->get_child_shape(south_child, shape), permutation, ns_i, chunk_end));
        }
        return ret;
    }

    /**
     * Convert a (possibly large) triples block into a quadtree.
     *
     * @tparam T
     * @tparam IT
     * @tparam CONFIG configuration to use. The subdivision threshold is CONFIG::leaf_split_threshold.
     * @param block triples block
     * @param shape shape of block
     * @return a quadmat quadtree where no leaf is larger than CONFIG::leaf_split_threshold
     */
    template <typename T, typename IT, typename CONFIG = default_config>
    tree_node_t<T, CONFIG> subdivide(const shared_ptr<triples_block<T, IT, CONFIG>> block, const shape_t& shape) {
        auto permutation = std::make_shared<typename triples_block<T, IT, CONFIG>::permutation_vector_type>(block->nnn());
        std::iota(permutation->begin(), permutation->end(), 0);
        return subdivide_impl(block, offset_t{0, 0}, shape, permutation, permutation->begin(), permutation->end());
    }
}

#endif //QUADMAT_TREE_CONSTRUCTION_H
