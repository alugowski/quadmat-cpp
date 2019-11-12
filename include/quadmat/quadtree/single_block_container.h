// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SINGLE_BLOCK_CONTAINER_H
#define QUADMAT_SINGLE_BLOCK_CONTAINER_H

#include "quadmat/quadtree/block_container.h"

namespace quadmat {

    /**
     * A simple block container that only contains a single child.
     *
     * @tparam T
     * @tparam CONFIG
     */
    template<typename T, typename CONFIG = default_config>
    class single_block_container : public block_container<T, CONFIG> {
    public:
        explicit single_block_container(const shape_t& shape) : shape(shape) {}
        single_block_container(const shape_t& shape, const tree_node_t<T, CONFIG> &child) : shape(shape), child(child) {}

        [[nodiscard]] size_t num_children() const override {
            return 1;
        }

        tree_node_t<T, CONFIG> get_child(int pos) const override {
            return child;
        }

        void set_child(int pos, tree_node_t<T, CONFIG> new_child) override {
            child = new_child;
        }

        std::shared_ptr<inner_block<T, CONFIG>> create_inner(int pos) override {
            std::shared_ptr<inner_block<T, CONFIG>> ret =
                    std::make_shared<inner_block<T, CONFIG>>(shape, get_discriminating_bit() >> 1);
            child = ret;
            return ret;
        }

        [[nodiscard]] offset_t get_offsets(int child_pos, const offset_t &my_offset) const override {
            return my_offset;
        }

        [[nodiscard]] shape_t get_child_shape(int child_pos, const shape_t &my_shape) const override {
            return my_shape;
        }

        /**
         * Pretend the child is in the NW position of an inner block. Discriminating bit should lie at the border or
         * beyond.
         */
        [[nodiscard]] index_t get_discriminating_bit() const override {
            return clear_all_except_msb(std::max(shape.ncols, shape.nrows) - 1) << 1;
        }
    protected:
        shape_t shape;
        tree_node_t<T, CONFIG> child;
    };
}

#endif //QUADMAT_SINGLE_BLOCK_CONTAINER_H
