// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SPA_H
#define QUADMAT_SPA_H

#include "quadmat/algorithms/dense_spa.h"
#include "quadmat/algorithms/map_spa.h"

namespace quadmat {

    /**
     * DenseSpa is fastest if memory allows. DenseSpa is O(n) where n is the size of the spa
     * (i.e. number of rows in the result).
     *
     * If memory does not allow then use a spa that is O(k) where k is the number of non-null elements in the spa.
     * MapSpa achieves this using std::map instead of an array like DenseSpa.
     *
     * Other implementations are possible.
     */
    template <typename IT, typename Semiring, typename Config=DefaultConfig>
    using SparseSpa = MapSpa<IT, Semiring, Config>;
}

#endif //QUADMAT_SPA_H
