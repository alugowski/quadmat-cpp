// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TREE_NODES_H
#define QUADMAT_TREE_NODES_H

#include <cstdint>
#include <variant>

#include "quadmat/quadtree/future_block.h"

namespace quadmat {

    // forward declaration of block types
    template<typename T, typename Config = DefaultConfig>
    class InnerBlock;

    template<typename T, typename IT, typename Config = DefaultConfig>
    class DcscBlock;

    template<typename IT, typename BaseLeaf>
    class WindowShadowBlock;

    /**
     * @brief Supported index sizes for leaf blocks
     *
     * The majority of QuadMat blocks have relatively small dimensions even if the entire matrix has large dimensions.
     * The type of (row, column) indices in QuadMat leaf blocks are a template parameter so that smaller blocks can
     * use smaller index types.
     */
    using LeafIndex = std::variant<int64_t, int32_t, int16_t>;

    /**
     * Leaf category type. Using a struct only because a struct template can be specialized.
     *
     * @tparam T data type
     * @tparam IT indexing type. This is specialized below.
     * @tparam Config configuration
     */
    template <typename T, typename IT, typename Config>
    struct LeafCategoryStruct {
    };

    /**
     * Leaf blocks with 64-bit indexing. These are all inter-operable with each other.
     */
    template <typename T, typename Config>
    struct LeafCategoryStruct<T, int64_t, Config> {
        using type = std::variant<
                std::shared_ptr<DcscBlock<T, int64_t, Config>>,
                std::shared_ptr<WindowShadowBlock<int64_t, DcscBlock<T, int16_t, Config>>>, // unused, needed for compilation of shadow blocks. TODO: find a way to get rid of this
                std::shared_ptr<WindowShadowBlock<int64_t, DcscBlock<T, int32_t, Config>>>, // unused, needed for compilation of shadow blocks. TODO: find a way to get rid of this
                std::shared_ptr<WindowShadowBlock<int64_t, DcscBlock<T, int64_t, Config>>>
                >;
    };

    /**
     * Leaf blocks with 32-bit indexing. These are all inter-operable with each other.
     */
    template <typename T, typename Config>
    struct LeafCategoryStruct<T, int32_t, Config> {
        using type = std::variant<
                std::shared_ptr<DcscBlock<T, int32_t, Config>>,
                std::shared_ptr<WindowShadowBlock<int32_t, DcscBlock<T, int16_t, Config>>>, // unused, needed for compilation of shadow blocks. TODO: find a way to get rid of this
                std::shared_ptr<WindowShadowBlock<int32_t, DcscBlock<T, int32_t, Config>>>,
                std::shared_ptr<WindowShadowBlock<int32_t, DcscBlock<T, int64_t, Config>>>
                >;
    };

    /**
     * Leaf blocks with 16-bit indexing. These are all inter-operable with each other.
     */
    template <typename T, typename Config>
    struct LeafCategoryStruct<T, int16_t, Config> {
        using type = std::variant<
                std::shared_ptr<DcscBlock<T, int16_t, Config>>,
                std::shared_ptr<WindowShadowBlock<int16_t, DcscBlock<T, int16_t, Config>>>,
                std::shared_ptr<WindowShadowBlock<int16_t, DcscBlock<T, int32_t, Config>>>,
                std::shared_ptr<WindowShadowBlock<int16_t, DcscBlock<T, int64_t, Config>>>
                >;
    };

    /**
     * This is the "retail" leaf category type that should be used instead of the structs.
     */
    template <typename T, typename IT, typename Config>
    using LeafCategory = typename LeafCategoryStruct<T, IT, Config>::type;

    /**
     * Leaf node
     */
    template <typename T, typename Config = DefaultConfig>
    using LeafNode = std::variant<
            LeafCategory<T, int64_t, Config>,
            LeafCategory<T, int32_t, Config>,
            LeafCategory<T, int16_t, Config>
            >;

    /**
     * All tree nodes
     */
    template <typename T, typename Config = DefaultConfig>
    using TreeNode = std::variant<
            std::monostate, // empty
            std::shared_ptr<FutureBlock<T, Config>>, // a block that will be computed later
            std::shared_ptr<InnerBlock<T, Config>>, // next level of quad tree
            LeafNode<T, Config> // leaf node
            >;

    /**
     * Determine what index type a leaf block should use.
     *
     * @param shape dimensions of leaf
     * @return a variant where the desired index type is set
     */
    inline LeafIndex GetLeafIndexType(const Shape& shape) {
        auto dim = std::max(shape.nrows, shape.ncols);

        if (dim <= std::numeric_limits<int16_t>::max()) {
            return static_cast<int16_t>(dim);
        } else if (dim <= std::numeric_limits<int32_t>::max()) {
            return static_cast<int32_t>(dim);
        } else {
            return static_cast<int64_t>(dim);
        }
    }
}

#endif //QUADMAT_TREE_NODES_H
