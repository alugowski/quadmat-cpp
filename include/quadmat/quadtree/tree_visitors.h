// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TREE_VISITORS_H
#define QUADMAT_TREE_VISITORS_H

#include "tree_nodes.h"
#include "inner_block.h"

namespace quadmat {

    /**
     * Helper class for writing std::variant visitors using lambdas.
     */
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
    template <typename T, typename CONFIG = basic_settings, typename GEN>
    tree_node_t<T, CONFIG> create_leaf(const shape_t& shape, const blocknnn_t nnn, const GEN col_ordered_gen) {
        leaf_index_type desired_index_type = get_leaf_index_type(shape);

        return std::visit(overloaded{
                [&](int64_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int64_t, CONFIG>(std::make_shared<dcsc_block<T, int64_t, CONFIG>>(shape, nnn, col_ordered_gen)); },
                [&](int32_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int32_t, CONFIG>(std::make_shared<dcsc_block<T, int32_t, CONFIG>>(shape, nnn, col_ordered_gen)); },
                [&](int16_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int16_t, CONFIG>(std::make_shared<dcsc_block<T, int16_t, CONFIG>>(shape, nnn, col_ordered_gen)); },
        }, desired_index_type);
    }

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
        explicit leaf_visitor_t(CALLBACK& leaf_callback) : leaf_callback(leaf_callback), offsets{} {}
        leaf_visitor_t(CALLBACK& leaf_callback, offset_t offsets) : leaf_callback(leaf_callback), offsets(offsets) {}

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
                std::visit(leaf_visitor_t<T, CALLBACK, CONFIG>(leaf_callback, child_offsets), child);
            }
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
        template <typename IT>
        void operator()(const std::shared_ptr<dcsc_block<T, IT, CONFIG>>& leaf) {
            if (leaf) {
                leaf_callback(offsets, leaf);
            }
        }

    protected:
        CALLBACK& leaf_callback;
        offset_t offsets;
    };


    /**
     * Factory method for leaf_visitor_t that deduces template parameters better than the leaf_visitor_t constructor.
     *
     * This overload is for non-const callbacks. In particular if the callback has a non-const operator().
     *
     * The callback needs to be thread safe.
     */
    template <typename T, typename CALLBACK, typename CONFIG = basic_settings>
    leaf_visitor_t<T, CALLBACK, CONFIG> leaf_visitor(CALLBACK& callback) {
        return leaf_visitor_t<T, CALLBACK, CONFIG>(callback);
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
    template <typename T, typename CALLBACK, typename CONFIG = basic_settings>
    leaf_visitor_t<T, const CALLBACK, CONFIG> leaf_visitor(const CALLBACK& callback) {
        return leaf_visitor_t<T, const CALLBACK, CONFIG>(callback);
    }
}

#endif //QUADMAT_TREE_VISITORS_H
