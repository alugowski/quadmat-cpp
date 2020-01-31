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
        static inline BlockNnn LeafSplitThreshold = 128u << 20u;

        /**
         * Maximum number of entries in a dense SpA. Larger problems use a sparse SpA.
         *
         * This is a constant here, but may not be const in other Configs.
         */
        static const std::size_t DenseSpaMaxCount = 100 * 1024 * 1024;

        /**
         * Largest size of a dense SpA's array. Larger problems use a sparse SpA.
         *
         * This is a constant here, but may not be const in other Configs.
         */
        static const std::size_t DenseSpaMaxBytes = 100  * 1024 * 1024;

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
         * @param max_estimated_flops an estimated upper bound on the number of expected FLOPs. Used so that we don't
         *                            waste time using a large dense SpA on only a handful of operations.
         * @return true if a dense SpA should be used, false if a sparse SpA should be used.
         */
        template <typename T>
        static bool ShouldUseDenseSpa(Index nrows, double max_estimated_flops) {
            // choose based on expected fill rate, size in element count or size in bytes
            return nrows * 0.001 < max_estimated_flops && nrows <= DenseSpaMaxCount && nrows * sizeof(T) <= DenseSpaMaxBytes;
        }

        /**
         * Whether or not to have a particular DCSC block use a column bitmask optimization.
         *
         * A common operation in matrix multiply is looking up columns in the A block. This operation has a low
         * hit rate. DCSC indices are generally slow, while a bitmask lookup is very fast. If memory is available
         * it may be worth to use a bitmask to mark whether columns are empty or not to speed up the common miss case.
         *
         * @param ncols number of columns in the block
         * @param num_nn_cols number of non-empty columns
         * @return true if it's ok to use a bitmask
         */
        static bool ShouldUseDcscBoolMask(Index ncols, std::size_t num_nn_cols) {
            std::size_t num_mask_bytes = ncols / 8;

            double column_fill_fraction = static_cast<double>(num_nn_cols) / ncols;

            return num_mask_bytes < 1u << 22u      // Only if (dense) mask doesn't use too much memory
                   && num_nn_cols > 1              // Only if block is not empty, where the regular search is fast
                   && column_fill_fraction < 0.9;  // Only if block is not nearly full, as mask only speeds up misses
        }

        /**
         * Whether or not to have a particular DCSC block use a dense CSC index optimization.
         *
         * A common operation in matrix multiply is looking up columns in the A block, something a CSC index is very
         * fast at.
         *
         * @param ncols number of columns in the block
         * @param num_nn_cols number of non-empty columns
         * @return true if it's ok to use a bitmask
         */
        static bool ShouldUseCscIndex(Index ncols, std::size_t num_nn_cols) {
            std::size_t num_bytes = ncols * sizeof(BlockNnn);

            return num_bytes < 1u << 26u      // Only if dense index doesn't use too much memory
                   && num_nn_cols > 1;        // Only if block is not empty, where the regular search is fast
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
    public:
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
    using DefaultConfig = TbbConfig;
}

#endif //QUADMAT_CONFIG_H
