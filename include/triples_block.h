// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TRIPLES_BLOCK_H
#define QUADMAT_TRIPLES_BLOCK_H

#include <numeric>
#include <vector>

using std::vector;

#include "block.h"

namespace quadmat {

    /**
     * A simple block composed of <row, column, value> triples
     *
     * @tparam T value type, eg. double
     */
    template<typename T, typename IT>
    class triples_block: public block<T> {
    public:

        /**
         * Add a triple.
         *
         * @param row row index
         * @param col column index
         * @param value value
         */
        void add(IT row, IT col, T value) {
            rows.push_back(row);
            cols.push_back(col);
            values.push_back(value);
        }

    protected:
        vector<IT> rows;
        vector<IT> cols;
        vector<T> values;

        template <typename Compare>
        vector<std::size_t> sort_permutation(
                const std::vector<T>& vec,
                Compare& compare)
        {
            std::vector<std::size_t> p(vec.size());
            std::iota(p.begin(), p.end(), 0);
            std::sort(p.begin(), p.end(),
                      [&](std::size_t i, std::size_t j){ return compare(vec[i], vec[j]); });
            return p;
        }
    };
}

#endif //QUADMAT_TRIPLES_BLOCK_H
