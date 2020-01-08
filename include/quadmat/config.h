// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_CONFIG_H
#define QUADMAT_CONFIG_H

#include <memory>
#include <numeric>

#include <tbb/tbb_allocator.h>

#include "quadmat/util/types.h"

namespace quadmat {

    class basic_config {
    public:
        /**
         * leaf blocks larger than this should be split
         */
        static const blocknnn_t leaf_split_threshold = 10 * 1024;

        /**
         * Maximum number of entries in a dense SpA. Larger problems use a sparse SpA.
         */
        static const size_t dense_spa_max_count = 100 * 1024 * 1024;

        /**
         * Largest size of a dense SpA's array. Larger problems use a sparse SpA.
         */
        static const size_t dense_spa_max_bytes = 10  * 1024 * 1024;

        /**
         * Decide whether to use a dense or sparse SpA.
         *
         * A dense SpA is an array, so lookups are fast. However for large matrices this can become unusable,
         * and a sparse SpA based on a map is a better choice.
         *
         * A dense SpA also requires the type to be default constructable.
         *
         * @tparam T type in the SpA
         * @param nrows size of the SpA
         * @return true if a dense SpA should be used, false if a sparse SpA should be used.
         */
        template <typename T>
        static bool use_dense_spa(size_t nrows) {
            // choose based on count or size in bytes
            return nrows <= dense_spa_max_count && nrows * sizeof(T) <= dense_spa_max_bytes;
        }

        /**
         * default allocator, and for long-lived objects
         */
        template <typename T>
        using ALLOC = std::allocator<T>;

        /**
         * allocator to use for short-lived temporary objects
         */
        template <typename T>
        using TEMP_ALLOC = std::allocator<T>;
    };

    class tbb_config : public basic_config {
        /**
         * default allocator, and for long-lived objects
         */
        template <typename T>
        using ALLOC = tbb::tbb_allocator<T>;

        /**
         * allocator to use for short-lived temporary objects
         */
        template <typename T>
        using TEMP_ALLOC = tbb::tbb_allocator<T>;
    };

    /**
     * This is the config that will be used by default unless another is explicitly specified as a template parameter.
     */
    using default_config = basic_config;
}

#endif //QUADMAT_CONFIG_H
