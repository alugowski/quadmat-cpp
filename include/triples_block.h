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
        triples_block(const index_t nrows, const index_t ncols) : block<T>(nrows, ncols) {}

        block_size_info size() override {
            return block_size_info{
                rows.size() * sizeof(IT) + cols.size() * sizeof(IT),
                values.size() * sizeof(T),
                2 * sizeof(std::vector<IT>) + sizeof(std::vector<T>)
            } + block<T>::size();
        }

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
