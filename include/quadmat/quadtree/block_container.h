// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BLOCK_CONTAINER_H
#define QUADMAT_BLOCK_CONTAINER_H

#include "quadmat/quadtree/tree_nodes.h"

namespace quadmat {

    /**
     * A container of blocks.
     *
     * @tparam T
     */
    template<typename T, typename CONFIG>
    class block_container {
    public:
        [[nodiscard]] virtual size_t num_children() const = 0;

        virtual tree_node_t<T, CONFIG> get_child(int pos) const = 0;

        virtual void set_child(int pos, tree_node_t<T, CONFIG> child) = 0;

        virtual std::shared_ptr<inner_block<T, CONFIG>> create_inner(int pos) = 0;

        [[nodiscard]] virtual offset_t get_offsets(int child_pos, const offset_t& my_offset) const = 0;

        [[nodiscard]] virtual shape_t get_child_shape(int child_pos, const shape_t& my_shape) const = 0;
    };
}
#endif //QUADMAT_BLOCK_CONTAINER_H
