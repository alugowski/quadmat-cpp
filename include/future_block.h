// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_FUTURE_BLOCK_H
#define QUADMAT_FUTURE_BLOCK_H

#include "block.h"

namespace quadmat {

    /**
     * A block placeholder for a position that has not yet been computed.
     *
     * @tparam T value type, eg. double
     */
    template<typename T, typename CONFIG>
    class future_block : public block<T> {
    public:
        explicit future_block(const shape_t shape) : block<T>(shape) {}
    };
}

#endif //QUADMAT_FUTURE_BLOCK_H
