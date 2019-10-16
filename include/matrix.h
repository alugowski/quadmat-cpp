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
        matrix(index_t nrow, index_t ncol): nrow(nrow), ncol(ncol) {}

        index_t get_nrow() const { return nrow; }
        index_t get_ncol() const { return ncol; }

        shared_ptr<block<T> > get_root() const { return shared_ptr<block<T> >(root); }
        void set_root(shared_ptr<block<T> > new_root) { root = new_root; }

    protected:
        index_t nrow;
        index_t ncol;

        shared_ptr<block<T> > root;
    };
}

#endif //QUADMAT_MATRIX_H
