// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_ALGORITHMS_H
#define QUADMAT_ALGORITHMS_H

#include "quadmat/matrix.h"
#include "quadmat/algorithms/multiply_trees.h"

namespace quadmat {

    /**
     * Multiply two matrices.
     *
     * @tparam Semiring semiring type
     * @tparam Config configuration to use
     * @param a left-hand side matrix
     * @param b right-hand side matrix
     * @param semiring semiring object ot use
     * @return a new matrix populated with the result of the multiplication
     */
    template <typename Semiring, typename Config>
    Matrix<typename Semiring::ReduceType, Config> Multiply(
            const Matrix<typename Semiring::MapTypeA, Config>& a,
            const Matrix<typename Semiring::MapTypeB, Config>& b,
            const Semiring& semiring = Semiring()) {

        // create the result matrix
        Matrix<typename Semiring::ReduceType, Config> ret = Matrix<typename Semiring::ReduceType, Config>{
            GetMultiplyResultShape(a.GetShape(), b.GetShape())
        };

        DirectTaskQueue<DefaultConfig> queue;

        // setup multiply job
        queue.Enqueue(quadmat::allocate_shared_temp<Config, MultiplyTask<Semiring, Config>>(
                queue,
                PairSet<typename Semiring::MapTypeA, typename Semiring::MapTypeB, Config>{
                        a.GetRootBC()->GetChild(0),
                    b.GetRootBC()->GetChild(0),
                        a.GetShape(),
                        b.GetShape(),
                    a.GetRootBC()->GetDiscriminatingBit(),
                    b.GetRootBC()->GetDiscriminatingBit()
                    },
                ret.GetRootBC(), 0, Offset{0, 0}, ret.GetShape(), semiring));

        return ret;
    }
}

#endif //QUADMAT_ALGORITHMS_H
