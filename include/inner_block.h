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

    template<typename T>
    class block_container {
    public:
        int num_children() const;
    };

    template<typename T>
    class inner_block : public block<T> {
    public:
        inner_block(const index_t nrows, const index_t ncols) : block<T>(nrows, ncols) {}

        block_size_info size() override {
            // sum sizes of children
            return std::accumulate(children.begin(), children.end(), block<T>::size(),
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
