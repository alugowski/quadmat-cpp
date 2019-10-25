// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TYPES_H
#define QUADMAT_TYPES_H

#include <numeric>

namespace quadmat {

    typedef int32_t blocknnn_t;
    typedef int64_t index_t;

    /**
     * Utility type to be able to return iterator ranges usable in a foreach.
     * @tparam ITER
     */
    template <typename ITER>
    struct range_t {
        ITER _begin;
        ITER _end;

        ITER begin() const { return _begin; }
        ITER end() const { return _end; }

        auto size() const {
            return _end - _begin;
        }
    };

    /**
     * Utility type that describes the shape of a block or matrix. I.e. number of rows and columns.
     */
    struct shape_t {
        const index_t nrows = 0;
        const index_t ncols = 0;

        bool operator==(const shape_t& rhs) const {
            return nrows == rhs.nrows && ncols == rhs.ncols;
        }
    };

    /**
     * Utility structure for getting block statistics
     */
    struct block_size_info {
        size_t index_bytes = 0;
        size_t value_bytes = 0;
        size_t overhead_bytes = 0;
        size_t nnn = 0;

        [[nodiscard]] size_t total_bytes() const {
            return index_bytes + value_bytes + overhead_bytes;
        }

        block_size_info operator+(const block_size_info& rhs) const {
            return block_size_info {
                    index_bytes + rhs.index_bytes,
                    value_bytes + rhs.value_bytes,
                    overhead_bytes + rhs.overhead_bytes,
                    nnn + rhs.nnn};
        }
    };
}

#endif //QUADMAT_TYPES_H
