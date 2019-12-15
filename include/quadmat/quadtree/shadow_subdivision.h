// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SHADOW_SUBDIVISION_H
#define QUADMAT_SHADOW_SUBDIVISION_H

#include "quadmat/quadtree/tree_visitors.h"

namespace quadmat {

    /**
     * A visitor that performs shadow subdivision of leaf blocks.
     * @tparam T
     * @tparam CONFIG
     */
    template <typename T, typename CONFIG>
    class subdivision_visitor_t {
    public:
        explicit subdivision_visitor_t(const shape_t& shape, const index_t parent_discriminating_bit) : node_shape(shape), parent_discriminating_bit(parent_discriminating_bit) {}

        /**
         * Reached a leaf block category.
         */
        std::shared_ptr<inner_block<T, CONFIG>> operator()(const leaf_category_t<T, int64_t, CONFIG>& leaf) {
            return std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category.
         */
        std::shared_ptr<inner_block<T, CONFIG>> operator()(const leaf_category_t<T, int32_t, CONFIG>& leaf) {
            return std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category.
         */
        std::shared_ptr<inner_block<T, CONFIG>> operator()(const leaf_category_t<T, int16_t, CONFIG>& leaf) {
            return std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block. Call visitor.
         */
        template <typename LEAF_TYPE>
        std::shared_ptr<inner_block<T, CONFIG>> operator()(const std::shared_ptr<LEAF_TYPE>& leaf) {
            index_t discriminating_bit = get_child_discriminating_bit(parent_discriminating_bit);

            // find the column splits
            auto begin_column = leaf->columns_begin();
            auto division_column = leaf->column_lower_bound(discriminating_bit);
            auto end_column = leaf->columns_end();

            std::shared_ptr<inner_block<T, CONFIG>> ret =
                    std::make_shared<inner_block<T, CONFIG>>(discriminating_bit);

            // create shadow block children
            for (inner_position child_pos : all_inner_positions) {
                offset_t child_offsets = ret->get_offsets(child_pos, {0, 0});
                shape_t child_shape = ret->get_child_shape(child_pos, node_shape);

                switch (child_pos) {
                    case NW:
                    case SW:
                        if (division_column != begin_column) {
                            ret->set_child(child_pos, leaf->get_shadow_block(leaf, begin_column, division_column,
                                                                             child_offsets, child_shape));
                        }
                        break;
                    case NE:
                    case SE:
                        if (division_column != end_column) {
                            ret->set_child(child_pos, leaf->get_shadow_block(leaf, division_column, end_column,
                                                                             child_offsets, child_shape));
                        }
                        break;
                }
            }

            return ret;
        }

    protected:
        const shape_t node_shape;
        const index_t parent_discriminating_bit;
    };

    /**
     * Perform shadow subdivision of a leaf block.
     * @tparam T
     * @tparam CONFIG
     * @param node block to subdivide. Should be a leaf.
     * @param shape shape of node
     * @return an inner block
     */
    template <typename T, typename CONFIG = default_config>
    std::shared_ptr<inner_block<T, CONFIG>> shadow_subdivide(const leaf_node_t<T, CONFIG>& node, const shape_t& shape, const index_t parent_discriminating_bit) {
        return std::visit(subdivision_visitor_t<T, CONFIG>(shape, parent_discriminating_bit), node);
    }
}

#endif //QUADMAT_SHADOW_SUBDIVISION_H
