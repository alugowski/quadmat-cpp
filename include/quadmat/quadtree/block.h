// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BLOCK_H
#define QUADMAT_BLOCK_H

#include "quadmat/config.h"

namespace quadmat {

    /**
     * Root class of all blocks.
     *
     * @tparam T value type, eg. double
     */
    template<typename T>
    class block {
    public:
        explicit block() = default;
        virtual ~block() = default;

        /**
         * @return byte size of this block along with rough breakdown between index, value, and other
         */
        virtual block_size_info size() {
            return block_size_info();
        }
    };
}

#endif //QUADMAT_BLOCK_H
