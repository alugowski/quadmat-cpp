// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_CONFIG_H
#define QUADMAT_CONFIG_H

#include <memory>
#include <numeric>

#include <tbb/tbb_allocator.h>

namespace quadmat {
    typedef int32_t blocknnn_t;
    typedef int64_t index_t;

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

    /**
     * Utility structure for getting block statistics
     */
    struct block_size_info {
        size_t index_bytes = 0;
        size_t value_bytes = 0;
        size_t overhead_bytes = 0;

        [[nodiscard]] size_t total_bytes() const {
            return index_bytes + value_bytes + overhead_bytes;
        }

        block_size_info operator+(const block_size_info& rhs) const {
            return block_size_info {
                index_bytes + rhs.index_bytes,
                value_bytes + rhs.value_bytes,
                overhead_bytes + rhs.overhead_bytes};
        }
    };
}

#endif //QUADMAT_CONFIG_H
