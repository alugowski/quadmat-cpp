// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_ALGORITHMS_H
#define QUADMAT_ALGORITHMS_H

#include "quadmat/matrix.h"
#include "quadmat/algorithms/multiply_trees.h"

namespace quadmat {

    /**
     * Multiply two matrices.
     *
     * @tparam SR semiring type
     * @tparam CONFIG configuration to use
     * @param a left-hand side matrix
     * @param b right-hand side matrix
     * @param semiring semiring object ot use
     * @return a new matrix populated with the result of the multiplication
     */
    template <typename SR, typename CONFIG>
    matrix<typename SR::reduce_type, CONFIG> multiply(
            const matrix<typename SR::map_type_l, CONFIG>& a,
            const matrix<typename SR::map_type_r, CONFIG>& b,
            const SR& semiring = SR()) {

        // create the result matrix
        matrix<typename SR::reduce_type, CONFIG> ret = matrix<typename SR::reduce_type, CONFIG>{shape_t{
            .nrows = a.get_shape().nrows,
            .ncols = b.get_shape().ncols
        }};

        // setup multiply job
        spawn_multiply_job<SR, CONFIG> job(
                quadmat::pair_set_t<typename SR::map_type_l, typename SR::map_type_r, CONFIG>{
                    a.get_root_bc()->get_child(0),
                    b.get_root_bc()->get_child(0)
                    },
                ret.get_root_bc(), 0, {0, 0}, ret.get_shape(), semiring);

        // run multiply
        job.run();

        return ret;
    }
}

#endif //QUADMAT_ALGORITHMS_H
