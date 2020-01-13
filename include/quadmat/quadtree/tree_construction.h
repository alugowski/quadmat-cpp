// Copyright (C) 2019-2020 Adam Lugowski
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
     * @tparam Config settings
     * @tparam Gen tuple generator. Must have a begin() and end() that return tuple<IT, IT, T> for some integer type IT
     * @param shape shape of leaf
     * @param nnn estimated number of non-nulls, i.e. col_ordered_gen.size().
     * @param col_ordered_gen tuple generator
     * @return
     */
    template <typename T, typename Config = DefaultConfig, typename Gen>
    LeafNode<T, Config> CreateLeaf(const Shape& shape, const BlockNnn nnn, const Gen col_ordered_gen) {
        LeafIndex desired_index_type = GetLeafIndexType(shape);

        return std::visit(overloaded{
                [&](int64_t dim) -> LeafNode<T, Config> {
                  return LeafCategory<T, int64_t, Config>(DcscBlockFactory<T, int64_t, Config>(nnn, col_ordered_gen).Finish());
                },
                [&](int32_t dim) -> LeafNode<T, Config> {
                  return LeafCategory<T, int32_t, Config>(DcscBlockFactory<T, int32_t, Config>(nnn, col_ordered_gen).Finish());
                },
                [&](int16_t dim) -> LeafNode<T, Config> {
                  return LeafCategory<T, int16_t, Config>(DcscBlockFactory<T, int16_t, Config>(nnn, col_ordered_gen).Finish());
                },
        }, desired_index_type);
    }

    /**
     * Implementation of triples block subdivision.
     */
    template <typename T, typename IT, typename Config>
    TreeNode<T, Config> SubdivideImpl(const std::shared_ptr<TriplesBlock<T, IT, Config>>& block,
                                      const Offset offsets, const Shape& shape, Index parent_discriminating_bit,
                                      std::shared_ptr<typename TriplesBlock<T, IT, Config>::PermutationVectorType> permutation,
                                      typename TriplesBlock<T, IT, Config>::PermutationVectorType::iterator perm_begin,
                                      typename TriplesBlock<T, IT, Config>::PermutationVectorType::iterator perm_end) {
        // if there are no triples then return an empty node
        if ((perm_end - perm_begin) == 0) {
            return TreeNode<T, Config>();
        }

        // if there are only one leaf's worth of triples then construct a single leaf
        if ((perm_end - perm_begin) < Config::LeafSplitThreshold) {
            // sort
            std::sort(perm_begin, perm_end,
                      [&](size_t i, size_t j) {
                          if (block->GetCol(i) != block->GetCol(j)) {
                              return block->GetCol(i) < block->GetCol(j);
                          } else if (block->GetRow(i) != block->GetRow(j)) {
                              return block->GetRow(i) < block->GetRow(j);
                          } else {
                              return i < j;
                          }
                      });

            return CreateLeaf<T, Config>(
                    shape,
                    (perm_end - perm_begin),
                    OffsetTuplesNeg(block->PermutedTuples(permutation, perm_begin, perm_end), offsets));
        }

        // subdivision is required
        const Index discriminating_bit = GetChildDiscriminatingBit(parent_discriminating_bit);
        auto ret = quadmat::allocate_shared<Config, InnerBlock<T, Config>>(discriminating_bit);

        // partition east and west children by column
        std::partition(perm_begin, perm_end,
                       [&](size_t i) -> bool {
                           return (block->GetCol(i) - offsets.col_offset < discriminating_bit);
                       });

        // find the east-west divide
        auto ew_i = std::lower_bound(
                perm_begin,
                perm_end,
                discriminating_bit,
                [&](size_t i, Index value) -> bool {
                    return block->GetCol(i) - offsets.col_offset < value;
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
                               return (block->GetRow(i) - offsets.row_offset < discriminating_bit);
                           });

            // find the north-south divide
            auto ns_i = std::lower_bound(
                    chunk_begin,
                    chunk_end,
                    discriminating_bit,
                    [&](size_t i, Index value) -> bool {
                        return block->GetRow(i) - offsets.row_offset < value;
                    });

            ret->SetChild(north_child,
                          SubdivideImpl(block,
                                        ret->GetChildOffsets(north_child, offsets),
                                        ret->GetChildShape(north_child, shape),
                                        discriminating_bit,
                                        permutation,
                                        chunk_begin,
                                        ns_i));
            ret->SetChild(south_child,
                          SubdivideImpl(block,
                                        ret->GetChildOffsets(south_child, offsets),
                                        ret->GetChildShape(south_child, shape),
                                        discriminating_bit,
                                        permutation,
                                        ns_i,
                                        chunk_end));
        }
        return ret;
    }

    /**
     * Convert a (possibly large) triples block into a quadtree.
     *
     * @tparam T
     * @tparam IT
     * @tparam Config configuration to use. The subdivision threshold is Config::LeafSplitThreshold.
     * @param block triples block
     * @param shape shape of block
     * @return a quadmat quadtree where no leaf is larger than Config::LeafSplitThreshold
     */
    template <typename T, typename IT, typename Config = DefaultConfig>
    TreeNode<T, Config> Subdivide(const std::shared_ptr<TriplesBlock<T, IT, Config>> block, const Shape& shape) {
        auto permutation = quadmat::allocate_shared<Config, typename TriplesBlock<T, IT, Config>::PermutationVectorType>(block->GetNnn());
        std::iota(permutation->begin(), permutation->end(), 0);
        return SubdivideImpl(block,
                             Offset{0, 0},
                             shape,
                             GetDiscriminatingBit(shape) << 1, // NOLINT(hicpp-signed-bitwise)
                             permutation,
                             permutation->begin(),
                             permutation->end());
    }
}

#endif //QUADMAT_TREE_CONSTRUCTION_H
