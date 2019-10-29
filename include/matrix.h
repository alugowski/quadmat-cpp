// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MATRIX_H
#define QUADMAT_MATRIX_H

#include <memory>

using std::shared_ptr;

#include "tree_nodes.h"
#include "config.h"

namespace quadmat {

    template <typename T, typename CONFIG = basic_settings>
    class matrix {
    public:
        explicit matrix(const shape_t shape): shape(shape) {}

        [[nodiscard]] shape_t get_shape() const { return shape; }

        tree_node_t<T, CONFIG> get_root() const { return shared_ptr<block<T> >(root); }
        void set_root(tree_node_t<T, CONFIG> new_root) { root = new_root; }

    protected:
        shape_t shape;

        tree_node_t<T, CONFIG> root;
    };
}

#endif //QUADMAT_MATRIX_H
