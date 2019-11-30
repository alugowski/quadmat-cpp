// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TREE_VISITORS_H
#define QUADMAT_TREE_VISITORS_H

#include "quadmat/quadtree/tree_nodes.h"
#include "quadmat/quadtree/inner_block.h"

namespace quadmat {

    /**
     * Helper class for writing std::variant visitors using lambdas.
     */
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    /**
     * Basic visitor that simply visits each leaf.
     *
     * Callback needs to overload operator()(offset_t, leaf)
     *
     * Callback needs to be thread safe.
     *
     * @tparam T
     * @tparam CALLBACK visitor callback implementation
     * @tparam CONFIG
     */
    template <typename T, typename CALLBACK, typename CONFIG>
    class leaf_visitor_t {
    public:
        leaf_visitor_t(CALLBACK& leaf_callback, const offset_t& offsets, const shape_t& shape) : leaf_callback(leaf_callback), offsets(offsets), shape(shape) {}

        /**
         * Reached a null block.
         */
        void operator()(const std::monostate& ignored) {
        }

        /**
         * Reached a future block. Nothing to do.
         */
        void operator()(const std::shared_ptr<future_block<T, CONFIG>>& fb) {
        }

        /**
         * Reached an inner block. Recurse on children and modify offsets accordingly.
         */
        void operator()(const std::shared_ptr<inner_block<T, CONFIG>> inner) {
            for (auto pos : all_inner_positions) {
                auto child = inner->get_child(pos);

                if (std::holds_alternative<std::monostate>(child)) {
                    continue;
                }

                offset_t child_offsets = inner->get_offsets(pos, offsets);
                shape_t child_shape = inner->get_child_shape(pos, shape);
                std::visit(leaf_visitor_t<T, CALLBACK, CONFIG>(leaf_callback, child_offsets, child_shape), child);
            }
        }

        /**
         * Reached a leaf node.
         */
        void operator()(const leaf_node_t<T, CONFIG>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category. Visit this variant to get the actual leaf.
         */
        void operator()(const leaf_category_t<T, int64_t, CONFIG>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category. Visit this variant to get the actual leaf.
         */
        void operator()(const leaf_category_t<T, int32_t, CONFIG>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category. Visit this variant to get the actual leaf.
         */
        void operator()(const leaf_category_t<T, int16_t, CONFIG>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block. Call visitor.
         */
        template <typename LEAF_TYPE>
        void operator()(const std::shared_ptr<LEAF_TYPE>& leaf) {
            if (leaf) {
                leaf_callback(leaf, offsets, shape);
            }
        }

    protected:
        CALLBACK& leaf_callback;
        const offset_t offsets;
        const shape_t shape;
    };


    /**
     * Factory method for leaf_visitor_t that deduces template parameters better than the leaf_visitor_t constructor.
     *
     * This overload is for non-const callbacks. In particular if the callback has a non-const operator().
     *
     * The callback needs to be thread safe.
     */
    template <typename T, typename CALLBACK, typename CONFIG = default_config>
    leaf_visitor_t<T, CALLBACK, CONFIG> leaf_visitor(CALLBACK& callback, const shape_t& shape = {-1, -1}) {
        return leaf_visitor_t<T, CALLBACK, CONFIG>(callback, offset_t{0, 0}, shape);
    }

    /**
     * Factory method for leaf_visitor_t that deduces template parameters better than the leaf_visitor_t constructor.
     *
     * This overload is for const callbacks, such as lambdas.
     *
     * The callback needs to be thread safe.
     *
     * @example Visit all tuples in a tree node `node`:
     * \code
std::visit(leaf_visitor<double>([](offset_t offsets, auto leaf) {
            for (auto tup : leaf->tuples()) {
                // row index is std::get<0>(tup) + offsets.row_offset
                // col index is std::get<1>(tup) + offsets.col_offset
            }
        }), node);
     * \endcode
     */
    template <typename T, typename CALLBACK, typename CONFIG = default_config>
    leaf_visitor_t<T, const CALLBACK, CONFIG> leaf_visitor(const CALLBACK& callback, const shape_t& shape = {-1, -1}) {
        return leaf_visitor_t<T, const CALLBACK, CONFIG>(callback, offset_t{0, 0}, shape);
    }
}

#endif //QUADMAT_TREE_VISITORS_H
