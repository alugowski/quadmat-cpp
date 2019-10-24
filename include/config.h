// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_CONFIG_H
#define QUADMAT_CONFIG_H

#include <memory>
#include <numeric>

#include <tbb/tbb_allocator.h>

#include "types.h"

namespace quadmat {

    class basic_settings {
    public:
        /**
         * leaf blocks larger than this should be split
         */
        static const blocknnn_t leaf_split_threshold = 10 * 1024;

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

    class tbb_settings : public basic_settings {
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
}

#endif //QUADMAT_CONFIG_H
