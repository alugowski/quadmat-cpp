// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MATRIX_GENERATORS_H
#define QUADMAT_MATRIX_GENERATORS_H

#include <quadmat/generators/identity_tuples_generator.h>
#include "quadmat/matrix.h"
#include "quadmat/quadtree/leaf_blocks/triples_block.h"

namespace quadmat {

    /**
     * Construct an identity matrix.
     *
     * @tparam T matrix value type
     * @param n number of rows and columns
     */
    template <typename T, typename CONFIG = basic_settings>
    matrix<T> identity(index_t n) {
        identity_tuples_generator<T, index_t> gen(n);
        quadmat::tree_node_t<T, CONFIG> node = quadmat::create_leaf<T, CONFIG>({n, n}, n, gen);

        matrix<T, CONFIG> ret({n, n});
        ret.set_root(node);
        return ret;
    }

}

#endif //QUADMAT_MATRIX_GENERATORS_H
