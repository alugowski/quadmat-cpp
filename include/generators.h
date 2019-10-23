// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_GENERATORS_H
#define QUADMAT_GENERATORS_H

#include "matrix.h"
#include "triples_block.h"

namespace quadmat {

    /**
     * Construct an identity matrix.
     *
     * @tparam T matrix value type
     * @param n number of rows and columns
     */
    template <typename T>
    matrix<T> identity(index_t n) {
        shared_ptr<triples_block<T, index_t>> accum = std::make_shared<triples_block<T, index_t> >(n, n);

        for (index_t i = 0; i < n; i++) {
            accum->add(i, i, 1);
        }

        matrix<T> ret(n, n);
        ret.set_root(accum);
        return ret;
    }

}

#endif //QUADMAT_GENERATORS_H
