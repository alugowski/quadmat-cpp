// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BLOCK_CONTAINER_H
#define QUADMAT_BLOCK_CONTAINER_H

#include "block.h"

namespace quadmat {

    /**
     * A container of blocks.
     *
     * @tparam T
     */
    template<typename T, typename CONFIG>
    class block_container {
    public:
        [[nodiscard]] virtual size_t num_children() const = 0;

//        virtual typename std::vector<std::shared_ptr<block<T>>, typename CONFIG::template ALLOC<std::shared_ptr<block<T>>>>::const_iterator begin() const = 0;
//
//        virtual typename std::vector<std::shared_ptr<block<T>>, typename CONFIG::template ALLOC<std::shared_ptr<block<T>>>>::const_iterator end() const = 0;
    };
}
#endif //QUADMAT_BLOCK_CONTAINER_H
