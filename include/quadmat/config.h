// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_CONFIG_H
#define QUADMAT_CONFIG_H

#include <memory>
#include <numeric>

#include <tbb/tbb_allocator.h>

#include "quadmat/util/types.h"

namespace quadmat {

    /**
     * Basic safe configuration.
     */
    class BasicConfig {
    public:
        /**
         * Leaf blocks larger than this should be split.
         *
         * This is a constant here, but may not be const in other Configs.
         */
        static const BlockNnn LeafSplitThreshold = 10 * 1024;

        /**
         * Maximum number of entries in a dense SpA. Larger problems use a sparse SpA.
         *
         * This is a constant here, but may not be const in other Configs.
         */
        static const size_t DenseSpaMaxCount = 100 * 1024 * 1024;

        /**
         * Largest size of a dense SpA's array. Larger problems use a sparse SpA.
         *
         * This is a constant here, but may not be const in other Configs.
         */
        static const size_t DenseSpaMaxBytes = 10  * 1024 * 1024;

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
        static bool ShouldUseDenseSpa(size_t nrows) {
            // choose based on count or size in bytes
            return nrows <= DenseSpaMaxCount && nrows * sizeof(T) <= DenseSpaMaxBytes;
        }

        /**
         * Default allocator, and for long-lived objects
         */
        template <typename T>
        using Allocator = std::allocator<T>;

        /**
         * Allocator to use for short-lived temporary objects
         */
        template <typename T>
        using TempAllocator = std::allocator<T>;
    };

    /**
     * Configuration for use with ThreadingBuildingBlocks.
     */
    class TbbConfig : public BasicConfig {
        /**
         * Default allocator, and for long-lived objects
         */
        template <typename T>
        using Allocator = tbb::tbb_allocator<T>;

        /**
         * Allocator to use for short-lived temporary objects
         */
        template <typename T>
        using TempAllocator = tbb::tbb_allocator<T>;
    };

    /**
     * This is the config that will be used by QuadMat operations. If another config is desired then it should be
     * explicitly specified as a template parameter to those operations.
     */
    using DefaultConfig = BasicConfig;
}

#endif //QUADMAT_CONFIG_H
