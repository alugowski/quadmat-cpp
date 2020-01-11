// Copyright (C) 2019-2020 Adam Lugowski
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
    template <typename T, typename Config = DefaultConfig>
    Matrix<T> Identity(Index n) {
        IdentityTuplesGenerator<T, Index> gen(n);
        TreeNode<T, Config> node = CreateLeaf<T, Config>({n, n}, n, gen);

        Matrix<T, Config> ret({n, n});
        ret.GetRootBC()->SetChild(0, node);
        return ret;
    }

}

#endif //QUADMAT_MATRIX_GENERATORS_H
