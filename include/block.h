// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BLOCK_H
#define QUADMAT_BLOCK_H

#include "config.h"

namespace quadmat {

    /**
     * Root class of all blocks.
     *
     * @tparam T value type, eg. double
     */
    template<typename T>
    class block {
    public:
        explicit block(const shape_t shape) : shape(shape) {}
        virtual ~block() = default;

        /**
         * @return this block's shape, i.e. number of rows and number of columns
         */
        [[nodiscard]] shape_t get_shape() const {
            return shape;
        }

        /**
         * @return byte size of this block along with rough breakdown between index, value, and other
         */
        virtual block_size_info size() {
            return block_size_info();
        }

    protected:
        const shape_t shape;
    };
}

#endif //QUADMAT_BLOCK_H
