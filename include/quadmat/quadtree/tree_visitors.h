// Copyright (C) 2019-2020 Adam Lugowski
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
     * @tparam Callback visitor callback implementation
     * @tparam Config
     */
    template <typename T, typename Callback, typename Config>
    class LeafVisitor {
    public:
        LeafVisitor(Callback& leaf_callback, const Offset& offsets, const Shape& shape) : leaf_callback_(leaf_callback), offsets_(offsets), shape_(shape) {}

        /**
         * Reached a null block.
         */
        void operator()(const std::monostate& ignored) {
        }

        /**
         * Reached a future block. Nothing to do.
         */
        void operator()(const std::shared_ptr<FutureBlock<T, Config>>& fb) {
        }

        /**
         * Reached an inner block. Recurse on children and modify offsets accordingly.
         */
        void operator()(const std::shared_ptr<InnerBlock<T, Config>> inner) {
            for (auto pos : kAllInnerPositions) {
                auto child = inner->GetChild(pos);

                if (std::holds_alternative<std::monostate>(child)) {
                    continue;
                }

                Offset child_offsets = inner->GetChildOffsets(pos, offsets_);
                Shape child_shape = inner->GetChildShape(pos, shape_);
                std::visit(LeafVisitor<T, Callback, Config>(leaf_callback_, child_offsets, child_shape), child);
            }
        }

        /**
         * Reached a leaf node.
         */
        void operator()(const LeafNode<T, Config>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category. Visit this variant to get the actual leaf.
         */
        void operator()(const LeafCategory<T, int64_t, Config>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category. Visit this variant to get the actual leaf.
         */
        void operator()(const LeafCategory<T, int32_t, Config>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block category. Visit this variant to get the actual leaf.
         */
        void operator()(const LeafCategory<T, int16_t, Config>& leaf) {
            std::visit(*this, leaf);
        }

        /**
         * Reached a leaf block. Call visitor.
         */
        template <typename LeafType>
        void operator()(const std::shared_ptr<LeafType>& leaf) {
            if (leaf) {
                leaf_callback_(leaf, offsets_, shape_);
            }
        }

    protected:
        Callback& leaf_callback_;
        const Offset offsets_;
        const Shape shape_;
    };


    /**
     * Factory method for leaf_visitor_t that deduces template parameters better than the leaf_visitor_t constructor.
     *
     * This overload is for non-const callbacks. In particular if the callback has a non-const operator().
     *
     * The callback needs to be thread safe.
     */
    template <typename T, typename Config = DefaultConfig, typename Callback>
    LeafVisitor<T, Callback, Config> GetLeafVisitor(Callback& callback, const Shape& shape = {-1, -1}) {
        return LeafVisitor<T, Callback, Config>(callback, Offset{0, 0}, shape);
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
std::visit(GetLeafVisitor<double>([](auto leaf, offset_t offsets, shape_t shape) {
            for (auto tup : leaf->tuples()) {
                // row index is std::get<0>(tup) + offsets.row_offset
                // col index is std::get<1>(tup) + offsets.col_offset
            }
        }), node);
     * \endcode
     *
     * @tparam T
     * @tparam Config
     * @tparam Callback
     * @param callback
     * @param shape shape of the node. This is optional, however if it is omitted then the shape argument to the callback will be invalid.
     * @return a visitor to pass to std::visit
     */
    template <typename T, typename Config = DefaultConfig, typename Callback>
    LeafVisitor<T, const Callback, Config> GetLeafVisitor(const Callback& callback, const Shape& shape = {-1, -1}) {
        return LeafVisitor<T, const Callback, Config>(callback, Offset{0, 0}, shape);
    }
}

#endif //QUADMAT_TREE_VISITORS_H
