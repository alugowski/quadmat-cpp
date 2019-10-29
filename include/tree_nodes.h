// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TREE_NODES_H
#define QUADMAT_TREE_NODES_H

#include <cstdint>
#include <variant>

#include "dcsc_block.h"

namespace quadmat {

    // forward declaration
    template<typename T, typename CONFIG = basic_settings>
    class inner_block;

    /**
     * @brief Supported index sizes for leaf blocks
     *
     * The majority of QuadMat blocks have relatively small dimensions even if the entire matrix has large dimensions.
     * The type of (row, column) indices in QuadMat leaf blocks are a template parameter so that smaller blocks can
     * use smaller index types.
     */
    using leaf_index_type = std::variant<int64_t, int32_t, int16_t>;

    /**
     * Leaf blocks with 64-bit indexing. These are all inter-operable with each other.
     */
    template <typename T, typename CONFIG>
    using leaf64_t = std::variant<std::shared_ptr<dcsc_block<T, int64_t, CONFIG>>>;

    /**
     * Leaf blocks with 32-bit indexing. These are all inter-operable with each other.
     */
    template <typename T, typename CONFIG>
    using leaf32_t = std::variant<std::shared_ptr<dcsc_block<T, int32_t, CONFIG>>>;

    /**
     * Leaf blocks with 16-bit indexing. These are all inter-operable with each other.
     */
    template <typename T, typename CONFIG>
    using leaf16_t = std::variant<std::shared_ptr<dcsc_block<T, int16_t, CONFIG>>>;

    /**
     * All tree nodes
     */
    template <typename T, typename CONFIG = basic_settings>
    using tree_node_t = std::variant<
            std::monostate,
            std::shared_ptr<inner_block<T, CONFIG>>,
            leaf64_t<T, CONFIG>,
            leaf32_t<T, CONFIG>,
            leaf16_t<T, CONFIG>
            >;

    /**
     * Determine what index type a leaf block should use.
     *
     * @param shape dimensions of leaf
     * @return a variant where the desired index type is set
     */
    inline leaf_index_type get_leaf_index_type(const shape_t& shape) {
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
