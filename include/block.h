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
        explicit block(const index_t nrows, const index_t ncols) : nrows(nrows), ncols(ncols) {}

        /**
         * @return number of rows
         */
        [[nodiscard]] index_t get_nrows() const {
            return nrows;
        }

        /**
         * @return number of columns
         */
        [[nodiscard]] index_t get_ncols() const {
            return ncols;
        }

        /**
         * @return byte size of this block along with rough breakdown between index, value, and other
         */
        virtual block_size_info size() {
            return block_size_info();
        }

    protected:
        const index_t nrows;
        const index_t ncols;
    };
}

#endif //QUADMAT_BLOCK_H
