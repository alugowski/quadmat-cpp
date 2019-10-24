// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MATRIX_H
#define QUADMAT_MATRIX_H

#include <memory>

using std::shared_ptr;

#include "block.h"
#include "config.h"

namespace quadmat {

    template <typename T>
    class matrix {
    public:
        matrix(const shape_t shape): shape(shape) {}

        shape_t get_shape() const { return shape; }

        shared_ptr<block<T> > get_root() const { return shared_ptr<block<T> >(root); }
        void set_root(shared_ptr<block<T> > new_root) { root = new_root; }

    protected:
        shape_t shape;

        shared_ptr<block<T> > root;
    };
}

#endif //QUADMAT_MATRIX_H
