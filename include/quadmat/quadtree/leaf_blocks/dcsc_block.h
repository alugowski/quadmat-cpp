// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DCSC_BLOCK_H
#define QUADMAT_DCSC_BLOCK_H

#include <algorithm>
#include <numeric>
#include <vector>

using std::vector;

#include "quadmat/quadtree/tree_nodes.h"
#include "quadmat/util/base_iterators.h"
#include "quadmat/util/util.h"

#include "quadmat/quadtree/leaf_blocks/window_shadow_block.h"

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
    template<typename T, typename IT, typename CONFIG>
    class dcsc_block: public block<T> {
    public:
        using value_type = T;
        using index_type = IT;
        using config_type = CONFIG;

        using row_iter_type = typename vector<IT, typename config_type::template ALLOC<IT>>::const_iterator;
        using value_iter_type = typename vector<value_type, typename config_type::template ALLOC<value_type>>::const_iterator;

        dcsc_block() = default;
    public:
        /**
         * Create a DCSC block. Use dcsc_block_factory to create these vectors.
         *
         * @param col_ind
         * @param col_ptr
         * @param row_ind
         * @param values
         */
        dcsc_block(vector<IT, typename CONFIG::template ALLOC<IT>>&& col_ind,
                   vector<blocknnn_t, typename CONFIG::template ALLOC<blocknnn_t>>&& col_ptr,
                   vector<IT, typename CONFIG::template ALLOC<IT>>&& row_ind,
                   vector<T, typename CONFIG::template ALLOC<T>>&& values) :
                   col_ind(col_ind), col_ptr(col_ptr), row_ind(row_ind), values(values) {}

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
         * Reference to a single column
         */
        struct column_ref {
            IT col;
            row_iter_type rows_begin;
            row_iter_type rows_end;
            value_iter_type values_begin;
        };

        /**
         * Iterator type for iterating over this block's columns
         */
        class column_iterator: public base_indexed_random_access_iterator<blocknnn_t, column_iterator> {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = column_ref;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            column_iterator(const dcsc_block<T, IT, CONFIG>* block, blocknnn_t i) : base_indexed_random_access_iterator<blocknnn_t, column_iterator>(i), block(block) {}
            column_iterator(const column_iterator& rhs) : base_indexed_random_access_iterator<blocknnn_t, column_iterator>(rhs.i), block(rhs.block) {}

            value_type operator*() const {
                return {
                    .col = block->col_ind[this->i],
                    .rows_begin = block->row_ind.begin() + block->col_ptr[this->i],
                    .rows_end = block->row_ind.begin() + block->col_ptr[this->i+1],
                    .values_begin = block->values.begin() + block->col_ptr[this->i],
                };
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
            return range_t<column_iterator>{columns_begin(), columns_end()};
        }

        /**
         * Get an iterator to a particular column.
         *
         * @param col column to look up
         * @return column iterator pointing at col, or columns_end()
         */
        column_iterator column(IT col) const {
            auto pos = std::lower_bound(begin(col_ind), end(col_ind), col);
            if (pos == end(col_ind) || *pos != col) {
                return columns_end();
            }
            return column_iterator(this, pos - begin(col_ind));
        }

        /**
         * @param col column to look up
         * @return column iterator point at col, or if there is no such column, the next larger column (or columns_end())
         */
        column_iterator column_lower_bound(IT col) const {
            auto pos = std::lower_bound(begin(col_ind), end(col_ind), col);
            if (pos == end(col_ind)) {
                return columns_end();
            }
            return column_iterator(this, pos - begin(col_ind));
        }

        [[nodiscard]] block_size_info size() const {
            return block_size_info{
                    col_ind.size() * sizeof(IT) + col_ptr.size() * sizeof(blocknnn_t) + row_ind.size() * sizeof(IT),
                    values.size() * sizeof(T),
                    sizeof(dcsc_block<T, IT, CONFIG>),
                    values.size()
            };
        }

        /**
         * @return number of tuples in this block.
         */
        [[nodiscard]] blocknnn_t nnn() const {
            return values.size();
        }

        /**
         * Create a shadow block that provides a view of a part of this leaf block
         */
        static tree_node_t<T, CONFIG> get_shadow_block(const std::shared_ptr<dcsc_block<T, IT, CONFIG>>& base_dcsc, const offset_t& offsets, const shape_t& shape) {
            // find the column range for the shadow
            auto begin_column_pos = std::lower_bound(begin(base_dcsc->col_ind), end(base_dcsc->col_ind), offsets.col_offset);
            auto begin_column = column_iterator(base_dcsc.get(), begin_column_pos - begin(base_dcsc->col_ind));

            auto end_column_pos = std::upper_bound(begin(base_dcsc->col_ind), end(base_dcsc->col_ind), offsets.col_offset - 1 + shape.ncols);
            auto end_column = column_iterator(base_dcsc.get(), end_column_pos - begin(base_dcsc->col_ind));

            leaf_index_type shadow_type = get_leaf_index_type(shape);

            return std::visit(overloaded{
                    [&](int64_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int64_t, CONFIG>(std::make_shared<window_shadow_block<int64_t, dcsc_block<T, IT, CONFIG>>>(base_dcsc, begin_column, end_column, offsets, shape)); },
                    [&](int32_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int32_t, CONFIG>(std::make_shared<window_shadow_block<int32_t, dcsc_block<T, IT, CONFIG>>>(base_dcsc, begin_column, end_column, offsets, shape)); },
                    [&](int16_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int16_t, CONFIG>(std::make_shared<window_shadow_block<int16_t, dcsc_block<T, IT, CONFIG>>>(base_dcsc, begin_column, end_column, offsets, shape)); },
            }, shadow_type);
        }

        /**
         * Create a shadow block that provides a view of a part of this leaf block
         *
         * @param base_dcsc a shared pointer to this
         * @param shadow_begin_column first column that the shadow block should consider
         * @param shadow_end_column one past the end column that the shadow block should consider
         * @param shadow_offsets row and column offsets of the shadow's tuples
         * @param shadow_shape shape of the shadow block
         * @return a leaf tree node
         */
        tree_node_t<T, CONFIG> get_shadow_block(const std::shared_ptr<dcsc_block<T, IT, CONFIG>>& base_dcsc,
                                                const column_iterator& shadow_begin_column, const column_iterator& shadow_end_column,
                                                const offset_t& shadow_offsets, const shape_t& shadow_shape) {
            leaf_index_type shadow_type = get_leaf_index_type(shadow_shape);

            return std::visit(overloaded{
                    [&](int64_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int64_t, CONFIG>(std::make_shared<window_shadow_block<int64_t, dcsc_block<T, IT, CONFIG>>>(base_dcsc, shadow_begin_column, shadow_end_column, shadow_offsets, shadow_shape)); },
                    [&](int32_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int32_t, CONFIG>(std::make_shared<window_shadow_block<int32_t, dcsc_block<T, IT, CONFIG>>>(base_dcsc, shadow_begin_column, shadow_end_column, shadow_offsets, shadow_shape)); },
                    [&](int16_t dim) -> tree_node_t<T, CONFIG> { return leaf_category_t<T, int16_t, CONFIG>(std::make_shared<window_shadow_block<int16_t, dcsc_block<T, IT, CONFIG>>>(base_dcsc, shadow_begin_column, shadow_end_column, shadow_offsets, shadow_shape)); },
            }, shadow_type);
        }

    protected:
        const vector<IT, typename CONFIG::template ALLOC<IT>> col_ind;
        const vector<blocknnn_t, typename CONFIG::template ALLOC<blocknnn_t>> col_ptr;
        const vector<IT, typename CONFIG::template ALLOC<IT>> row_ind;
        const vector<T, typename CONFIG::template ALLOC<T>> values;
    };

    /**
     * A tool to construct a DCSC block column by column. DCSC blocks are immutable after construction.
     *
     * @tparam T
     * @tparam IT
     * @tparam CONFIG
     */
    template<typename T, typename IT, typename CONFIG = default_config>
    class dcsc_block_factory {
    public:
        dcsc_block_factory() = default;

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
        dcsc_block_factory(const blocknnn_t nnn, const GEN col_ordered_gen) {
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
        }

        /**
         * Dump the contents of a SpA as the last column of the dcsc_block.
         *
         * @param col column to write the spa as. Must be greater than any column added so far.
         * @param spa
         */
        template <class SPA>
        void add_spa(IT col, SPA& spa) {
            // col > prev_col

            if (spa.empty()) {
                return;
            }

            col_ind.emplace_back(col);
            col_ptr.emplace_back(row_ind.size());

            spa.emplace_back_result(row_ind, values);
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
            col_ptr.emplace_back(row_ind.size());

            return std::make_shared<dcsc_block<T, IT, CONFIG>>(
                    std::move(col_ind),
                    std::move(col_ptr),
                    std::move(row_ind),
                    std::move(values)
                    );
        }

    protected:
        vector<IT, typename CONFIG::template ALLOC<IT>> col_ind;
        vector<blocknnn_t, typename CONFIG::template ALLOC<blocknnn_t>> col_ptr;
        vector<IT, typename CONFIG::template ALLOC<IT>> row_ind;
        vector<T, typename CONFIG::template ALLOC<T>> values;
    };
}

#endif //QUADMAT_DCSC_BLOCK_H
