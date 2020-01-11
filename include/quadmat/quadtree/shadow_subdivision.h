// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SHADOW_SUBDIVISION_H
#define QUADMAT_SHADOW_SUBDIVISION_H

#include "quadmat/quadtree/tree_visitors.h"

namespace quadmat {

    /**
     * A visitor that performs shadow subdivision of leaf blocks.
     * @tparam T
     * @tparam Config
     */
    template <typename T, typename Config>
    class subdivision_visitor_t {
    public:
        explicit subdivision_visitor_t(const Shape& shape, const Index parent_discriminating_bit) : node_shape(shape), parent_discriminating_bit(parent_discriminating_bit) {}

        /**
         * Reached a leaf block category.
         */
        std::shared_ptr<InnerBlock<T, Config>> operator()(const LeafCategory<T, int64_t, Config>& leaf) {
            return std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category.
         */
        std::shared_ptr<InnerBlock<T, Config>> operator()(const LeafCategory<T, int32_t, Config>& leaf) {
            return std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category.
         */
        std::shared_ptr<InnerBlock<T, Config>> operator()(const LeafCategory<T, int16_t, Config>& leaf) {
            return std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block. Call visitor.
         */
        template <typename LEAF_TYPE>
        std::shared_ptr<InnerBlock<T, Config>> operator()(const std::shared_ptr<LEAF_TYPE>& leaf) {
            Index discriminating_bit = GetChildDiscriminatingBit(parent_discriminating_bit);

            // find the column splits
            auto begin_column = leaf->ColumnsBegin();
            auto division_column = leaf->GetColumnLowerBound(discriminating_bit);
            auto end_column = leaf->ColumnsEnd();

            std::shared_ptr<InnerBlock<T, Config>> ret =
                    std::make_shared<InnerBlock<T, Config>>(discriminating_bit);

            // create shadow block children
            for (InnerPosition child_pos : kAllInnerPositions) {
                Offset child_offsets = ret->GetChildOffsets(child_pos, {0, 0});
                Shape child_shape = ret->GetChildShape(child_pos, node_shape);

                switch (child_pos) {
                    case NW:
                    case SW:
                        if (division_column != begin_column) {
                            ret->SetChild(child_pos, leaf->GetShadowBlock(leaf, begin_column, division_column,
                                                                           child_offsets, child_shape));
                        }
                        break;
                    case NE:
                    case SE:
                        if (division_column != end_column) {
                            ret->SetChild(child_pos, leaf->GetShadowBlock(leaf, division_column, end_column,
                                                                           child_offsets, child_shape));
                        }
                        break;
                }
            }

            return ret;
        }

    protected:
        const Shape node_shape;
        const Index parent_discriminating_bit;
    };

    /**
     * Perform shadow subdivision of a leaf block.
     * @tparam T
     * @tparam Config
     * @param node block to subdivide. Should be a leaf.
     * @param shape shape of node
     * @return an inner block
     */
    template <typename T, typename Config = DefaultConfig>
    std::shared_ptr<InnerBlock<T, Config>> shadow_subdivide(const LeafNode<T, Config>& node, const Shape& shape, const Index parent_discriminating_bit) {
        return std::visit(subdivision_visitor_t<T, Config>(shape, parent_discriminating_bit), node);
    }
}

#endif //QUADMAT_SHADOW_SUBDIVISION_H
