// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SINGLE_BLOCK_CONTAINER_H
#define QUADMAT_SINGLE_BLOCK_CONTAINER_H

#include "quadmat/quadtree/block_container.h"
#include "quadmat/util/util.h"

namespace quadmat {

    /**
     * A simple block container that only contains a single child.
     *
     * @tparam T
     * @tparam Config
     */
    template<typename T, typename Config = DefaultConfig>
    class single_block_container : public BlockContainer<T, Config> {
    public:
        explicit single_block_container(const Shape& shape) : shape(shape) {}
        single_block_container(const Shape& shape, const TreeNode<T, Config> &child) : shape(shape), child(child) {}

        [[nodiscard]] size_t GetNumChildren() const override {
            return 1;
        }

        TreeNode<T, Config> GetChild(int pos) const override {
            return child;
        }

        void SetChild(int pos, TreeNode<T, Config> new_child) override {
            child = new_child;
        }

        std::shared_ptr<InnerBlock<T, Config>> CreateInner(int pos) override {
            std::shared_ptr<InnerBlock<T, Config>> ret =
                    std::make_shared<InnerBlock<T, Config>>(GetDiscriminatingBit() >> 1);
            child = ret;
            return ret;
        }

        [[nodiscard]] Offset GetChildOffsets(int child_pos, const Offset &my_offset) const override {
            return my_offset;
        }

        [[nodiscard]] Shape GetChildShape(int child_pos, const Shape &my_shape) const override {
            return my_shape;
        }

        /**
         * Pretend the child is in the NW position of an inner block. Discriminating bit should lie at the border or
         * beyond.
         */
        [[nodiscard]] Index GetDiscriminatingBit() const override {
            Index dim_max = std::max(shape.ncols, shape.nrows);
            if (dim_max < 2) {
                return 1;
            }
            return quadmat::GetDiscriminatingBit(shape) << 1;
        }
    protected:
        Shape shape;
        TreeNode<T, Config> child;
    };
}

#endif //QUADMAT_SINGLE_BLOCK_CONTAINER_H
