// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INNER_BLOCK_H
#define QUADMAT_INNER_BLOCK_H

#include <algorithm>
#include <array>

using std::array;
using std::shared_ptr;

#include "block.h"

namespace quadmat {

    /**
     * Block for inner nodes of the quad tree.
     *
     * @tparam T value type of leaf nodes
     */
    template<typename T>
    class inner_block : public block<T> {
    public:
        explicit inner_block(const shape_t shape) : block<T>(shape) {}

        block_size_info size() override {
            // sum sizes of children
            return std::accumulate(children.begin(), children.end(),
                                   block_size_info{
                                           .overhead_bytes = sizeof(inner_block<T>),
                                   },
                                   [&](block_size_info acc, shared_ptr<block<T>>& child) {
                                       return acc + child->size();
                                   }
            );
        }

    protected:
        array<shared_ptr<block<T> >, 4> children;
    };
}

#endif //QUADMAT_INNER_BLOCK_H
