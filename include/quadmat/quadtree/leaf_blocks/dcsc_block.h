// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DCSC_BLOCK_H
#define QUADMAT_DCSC_BLOCK_H

#include <algorithm>
#include <numeric>
#include <vector>

using std::vector;

#include "quadmat/quadtree/block.h"
#include "quadmat/base_iterators.h"
#include "quadmat/util.h"


namespace quadmat {

    /**
     * Forward declaration of a type that is allowed to construct dcsc_blocks
     */
    template <typename T, typename IT, typename CONFIG = basic_settings>
    class dcsc_block_factory;

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
    public:
        friend class dcsc_block_factory<T, IT, CONFIG>;

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

            col_ptr.emplace_back(i);
        }

        /**
         * Iterator type for iterating over this block's tuples
         */
        class tuple_iterator: public base_indexed_random_access_iterator<size_t, tuple_iterator> {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            tuple_iterator(const dcsc_block<T, IT, CONFIG>* block, size_t i, IT col_idx) : base_indexed_random_access_iterator<size_t, tuple_iterator>(i), block(block), col_idx(col_idx) {}
            tuple_iterator(const tuple_iterator& rhs) : base_indexed_random_access_iterator<size_t, tuple_iterator>(rhs.i), block(rhs.block), col_idx(rhs.col_idx) {}

            value_type operator*() {
                return std::tuple<IT, IT, T>(block->row_ind[this->i], block->col_ind[col_idx], block->values[this->i]);
            }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
            tuple_iterator& operator++() {
                ++this->i;
                if (this->i == block->col_ptr[col_idx+1]) {
                    // new column
                    ++col_idx;
                }
                return *this;
            }

            tuple_iterator& operator+=(std::ptrdiff_t n) {
                this->i += n;
                // find which column we are now in
                auto col_greater = std::upper_bound(begin(block->col_ptr), end(block->col_ptr), this->i);
                col_idx = (col_greater - begin(block->col_ptr)) - 1;
                return *this;
            }
#pragma clang diagnostic pop

        private:
            const dcsc_block<T, IT, CONFIG>* block;
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
                tuple_iterator(this, 0, 0),
                tuple_iterator(this, row_ind.size(), col_ptr.size())
            };
        }

        /**
         * Iterator type for iterating over this block's columns
         */
        class column_iterator: public base_indexed_random_access_iterator<blocknnn_t, column_iterator> {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = IT;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            column_iterator(const dcsc_block<T, IT, CONFIG>* block, blocknnn_t i) : base_indexed_random_access_iterator<blocknnn_t, column_iterator>(i), block(block) {}
            column_iterator(const column_iterator& rhs) : base_indexed_random_access_iterator<blocknnn_t, column_iterator>(rhs.i), block(rhs.block) {}

            value_type operator*() const {
                return block->col_ind[this->i];
            }

            value_type operator[](const difference_type n) const {
                return block->col_ind[this->i + n];
            }

            [[nodiscard]] blocknnn_t get_col_idx() const {
                return this->i;
            }

            auto rows_begin() const {
                return block->row_ind.begin() + block->col_ptr[this->i];
            }
            auto rows_end() const {
                return block->row_ind.begin() + block->col_ptr[this->i+1];
            }

            auto values_begin() const {
                return block->values.begin() + block->col_ptr[this->i];
            }
            auto values_end() const {
                return block->values.begin() + block->col_ptr[this->i+1];
            }

        private:
            const dcsc_block<T, IT, CONFIG>* block;
        };

        /**
         * @return a column_iterator pointing at the first column
         */
        column_iterator columns_begin() const {
            return column_iterator(this, 0);
        }

        /**
         * @return a column_iterator pointing one past the last column
         */
        column_iterator columns_end() const {
            return column_iterator(this, col_ind.size());
        }

        /**
         * @return a {begin, end} pair of iterators to iterate over all the columns of this block.
         */
        range_t<column_iterator> columns() const {
            return range_t<column_iterator>{column_iterator(this, 0), column_iterator(this, col_ind.size())};
        }

        /**
         * Get an iterator to a particular column.
         *
         * @param col column to look up
         * @return column iterator pointing at col, or columns_end()
         */
        column_iterator column(IT col) const {
            auto pos = std::lower_bound(begin(col_ind), end(col_ind), col);
            return column_iterator(this, pos - begin(col_ind));
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

    /**
     * A tool to construct a DCSC block column by column. DCSC blocks are immutable after construction.
     *
     * @tparam T
     * @tparam IT
     * @tparam CONFIG
     */
    template<typename T, typename IT, typename CONFIG>
    class dcsc_block_factory {
    public:
        explicit dcsc_block_factory(const shape_t &shape) : ret(std::make_shared<dcsc_block<T, IT, CONFIG>>(shape)) {}

        /**
         * Dump the contents of a SpA as the last column of the dcsc_block.
         *
         * @param col column to write the spa as. Must be greater than any column added so far.
         * @param spa
         */
        template <class SPA>
        void add_spa(IT col, SPA& spa) {
            // col > prev_col

            ret->col_ind.emplace_back(col);
            ret->col_ptr.emplace_back(ret->row_ind.size());

            spa.emplace_back_result(ret->row_ind, ret->values);
        }

        /**
         * Call to return the constructed block. Also caps the data structures.
         *
         * No further calls to any methods in this class allowed after calling this method.
         *
         * @return the constructed dcsc_block
         */
        std::shared_ptr<dcsc_block<T, IT, CONFIG>> finish() {
            // cap off the columns
            if (ret->row_ind.size() != 0) {
                ret->col_ptr.emplace_back(ret->row_ind.size());
            }
            return ret;
        }

    protected:
        std::shared_ptr<dcsc_block<T, IT, CONFIG>> ret;
    };
}

#endif //QUADMAT_DCSC_BLOCK_H
