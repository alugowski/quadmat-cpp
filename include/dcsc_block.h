// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DCSC_BLOCK_H
#define QUADMAT_DCSC_BLOCK_H

#include <algorithm>
#include <numeric>
#include <vector>

using std::vector;

#include "block.h"
#include "generators/base_tuple_iterators.h"
#include "util.h"

namespace quadmat {

    /**
     * A Doubly-Compressed Sparse Columns block.
     *
     * This format is similar to CSC, but the column array is again compressed to not allow empty columns.
     * Only columns with at least a single non-null are represented.
     *
     * @tparam T value type
     * @tparam IT index type
     * @tparam CONFIG storage configuration options
     */
    template<typename T, typename IT, typename CONFIG = basic_settings>
    class dcsc_block: public block<T> {
    public:
        explicit dcsc_block(const shape_t shape) : block<T>(shape) {}

        /**
         * Create a DCSC block from (column, row)-ordered tuples. Single pass.
         *
         * @tparam GEN tuple generator template type
         * @param nrows number of rows in this block
         * @param ncols number of columns in this block
         * @param nnn estimated number of nonzeros in this block.
         * @param col_ordered_gen tuple generator. **Must return tuples ordered by column, row**
         */
        template<typename GEN>
        dcsc_block(const shape_t shape, const blocknnn_t nnn, const GEN col_ordered_gen) : block<T>(shape) {
            // reserve memory
            row_ind.reserve(nnn);
            values.reserve(nnn);

            IT prev_col = -1;
            blocknnn_t i = 0;
            for (auto tup : col_ordered_gen) {
                IT col = std::get<1>(tup);
                if (prev_col != col) {
                    // new column
                    col_ind.emplace_back(col);
                    col_ptr.emplace_back(i);
                }
                prev_col = col;

                row_ind.emplace_back(std::get<0>(tup)); // add row
                values.emplace_back(std::get<2>(tup)); // add value
                i++;
            }

            col_ind.emplace_back(prev_col); // duplicate index of last column to make iteration easier
            col_ptr.emplace_back(i);
        }

        /**
         * Input Iterator type for iterating over this block's tuples
         */
        class tuple_iterator: public base_tuple_input_iterator<tuple_iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            tuple_iterator(const dcsc_block<T, IT, CONFIG>& block, size_t i, IT col_idx) : block(block), i(i), col_idx(col_idx) {}
            tuple_iterator(const tuple_iterator& rhs) : block(rhs.block), i(rhs.i), col_idx(rhs.col_idx) {}

            value_type operator*() {
                return std::tuple<IT, IT, T>(block.row_ind[i], block.col_ind[col_idx], block.values[i]);
            }

            tuple_iterator& operator++() {
                ++i;
                if (i == block.col_ptr[col_idx+1]) {
                    // new column
                    ++col_idx;
                }
                return *this;
            }

            tuple_iterator& operator+=(int n) {
                i += n;
                // find which column we are now in
                auto col_greater = std::upper_bound(begin(block.col_ptr), end(block.col_ptr), i);
                col_idx = (col_greater - begin(block.col_ptr)) - 1;
                return *this;
            }

            bool operator==(tuple_iterator rhs) const {
                return i == rhs.i;
            }

        private:
            const dcsc_block<T, IT, CONFIG>& block;

            /**
             * Index of current row/value
             */
            size_t i;

            /**
             * Index of current column
             */
            IT col_idx;
        };

        /**
         * @return a (begin, end) tuple of iterators that iterate over values in this block and return tuples.
         */
        range_t<tuple_iterator> tuples() const {
            return range_t<tuple_iterator>{
                tuple_iterator(*this, 0, 0),
                tuple_iterator(*this, row_ind.size(), col_ptr.size())
            };
        }

        block_size_info size() override {
            return block_size_info{
                    col_ind.size() * sizeof(IT) + col_ptr.size() * sizeof(blocknnn_t) + row_ind.size() * sizeof(IT),
                    values.size() * sizeof(T),
                    sizeof(dcsc_block<T, IT, CONFIG>),
                    values.size()
            };
        }
    protected:
        vector<IT, typename CONFIG::template ALLOC<IT>> col_ind;
        vector<blocknnn_t, typename CONFIG::template ALLOC<blocknnn_t>> col_ptr;
        vector<IT, typename CONFIG::template ALLOC<IT>> row_ind;
        vector<T, typename CONFIG::template ALLOC<T>> values;
    };
}

#endif //QUADMAT_DCSC_BLOCK_H
