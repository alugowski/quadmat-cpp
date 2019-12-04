// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_WINDOW_SHADOW_BLOCK_H
#define QUADMAT_WINDOW_SHADOW_BLOCK_H

#include "quadmat/quadtree/block.h"
#include "quadmat/util/base_iterators.h"
#include "quadmat/util/util.h"

namespace quadmat {

    /**
     *
     * @tparam IT
     * @tparam BASE_LEAF
     */
    template<typename IT, typename BASE_LEAF>
    class window_shadow_block: public block<typename BASE_LEAF::value_type> {
    public:
        using value_type = typename BASE_LEAF::value_type;
        using index_type = IT;
        using config_type = typename BASE_LEAF::config_type;

        window_shadow_block(
                const std::shared_ptr<BASE_LEAF> &base,
                const typename BASE_LEAF::column_iterator& begin_column,
                const typename BASE_LEAF::column_iterator& end_column,
                const offset_t &offsets,
                const shape_t &shape)
                : base(base),
                  begin_column(begin_column),
                  end_column(end_column),
                  end_iter(column_iterator(this, end_column)),
                  offsets(offsets),
                  row_begin(offsets.row_offset), row_inclusive_end(offsets.row_offset - 1 + shape.nrows) {}

        /**
         * Reference to a single column
         */
        struct column_ref {
            IT col;
            offset_iterator<typename BASE_LEAF::row_iter_type> rows_begin;
            offset_iterator<typename BASE_LEAF::row_iter_type> rows_end;
            typename BASE_LEAF::value_iter_type values_begin;
        };

        /**
         * Iterator type for iterating over this block's columns
         */
        class column_iterator: public base_input_iterator<column_iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = column_ref;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            explicit column_iterator(const window_shadow_block<IT, BASE_LEAF>* shadow_block, const typename BASE_LEAF::column_iterator& iter) : shadow_block(shadow_block), iter(iter) {
                // It's possible that the given iter points at a column that doesn't have any rows that fit the window.
                advance_to_next_nonempty_col();
            }
            column_iterator(const column_iterator& rhs) : shadow_block(rhs.shadow_block), iter(rhs.iter) {}

            value_type operator*() const {
                auto base_ref = *iter;
                auto rows_begin = std::lower_bound(base_ref.rows_begin, base_ref.rows_end, shadow_block->row_begin);
                auto rows_end = std::upper_bound(rows_begin, base_ref.rows_end, shadow_block->row_inclusive_end);
                return {
                        .col = static_cast<IT>(base_ref.col - shadow_block->offsets.col_offset),
                        .rows_begin = offset_iterator<typename BASE_LEAF::row_iter_type>(rows_begin, -shadow_block->offsets.row_offset),
                        .rows_end = offset_iterator<typename BASE_LEAF::row_iter_type>(rows_end, -shadow_block->offsets.row_offset),
                        .values_begin = base_ref.values_begin + (rows_begin - base_ref.rows_begin),
                };
            }

            column_iterator& operator++() {
                do {
                    ++iter;
                } while (iter != shadow_block->end_column && !shadow_block->are_rows_in_window(iter));
                return *this;
            }

            bool operator==(const column_iterator& rhs) const {
                return iter == rhs.iter;
            }

            /**
             * @return iterator to the base column
             */
            const typename BASE_LEAF::column_iterator& get_base_iter() const {
                return iter;
            }

        private:
            void advance_to_next_nonempty_col() {
                while (iter != shadow_block->end_column && !shadow_block->are_rows_in_window(iter)) {
                    ++iter;
                }
            }
            const window_shadow_block<IT, BASE_LEAF>* shadow_block;
            typename BASE_LEAF::column_iterator iter;
        };

        /**
         * @return a column_iterator pointing at the first column
         */
        column_iterator columns_begin() const {
            return column_iterator(this, begin_column);
        }

        /**
         * @return a column_iterator pointing one past the last column
         */
        column_iterator columns_end() const {
            return end_iter;
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
            const typename BASE_LEAF::column_iterator base_iter = base->column(col + offsets.col_offset);

            if (base_iter == base->columns_end() || !are_rows_in_window(base_iter)) {
                return end_iter;
            } else {
                return column_iterator(this, base_iter);
            }
        }

        /**
         * @param col column to look up
         * @return column iterator point at col, or if there is no such column, the next larger column (or columns_end())
         */
        column_iterator column_lower_bound(IT col) const {
            const typename BASE_LEAF::column_iterator base_iter = base->column_lower_bound(col + offsets.col_offset);

            if (base_iter == base->columns_end()) {
                return end_iter;
            } else {
                return column_iterator(this, base_iter);
            }
        }

        /**
         * Warning: this method is O(n).
         *
         * @return number of tuples in this block.
         */
        [[nodiscard]] blocknnn_t nnn() const {
            blocknnn_t ret = 0;
            for (auto col : columns()) {
                ret += col.rows_end - col.rows_begin;
            }
            return ret;
        }

        /**
         * Create a shadow block that provides a view of a part of this leaf block
         *
         * @param ignored shared pointer to this.
         * @param shadow_begin_column first column that the shadow block should consider
         * @param shadow_end_column one past the end column that the shadow block should consider
         * @param shadow_offsets row and column offsets of the shadow's tuples
         * @param shadow_shape shape of the shadow block
         * @return a leaf tree node
         */
        tree_node_t<typename BASE_LEAF::value_type, typename BASE_LEAF::config_type> get_shadow_block(
                const std::shared_ptr<window_shadow_block<IT, BASE_LEAF>>& ignored,
                const column_iterator& shadow_begin_column, const column_iterator& shadow_end_column,
                const offset_t& shadow_offsets, const shape_t& shadow_shape) {

            leaf_index_type shadow_type = get_leaf_index_type(shadow_shape);

            return std::visit(overloaded{
                    [&](int64_t dim) -> tree_node_t<typename BASE_LEAF::value_type, typename BASE_LEAF::config_type> { return leaf_category_t<typename BASE_LEAF::value_type, int64_t, typename BASE_LEAF::config_type>(std::make_shared<window_shadow_block<int64_t, BASE_LEAF>>(base, shadow_begin_column.get_base_iter(), shadow_end_column.get_base_iter(), shadow_offsets, shadow_shape)); },
                    [&](int32_t dim) -> tree_node_t<typename BASE_LEAF::value_type, typename BASE_LEAF::config_type> { return leaf_category_t<typename BASE_LEAF::value_type, int32_t, typename BASE_LEAF::config_type>(std::make_shared<window_shadow_block<int32_t, BASE_LEAF>>(base, shadow_begin_column.get_base_iter(), shadow_end_column.get_base_iter(), shadow_offsets, shadow_shape)); },
                    [&](int16_t dim) -> tree_node_t<typename BASE_LEAF::value_type, typename BASE_LEAF::config_type> { return leaf_category_t<typename BASE_LEAF::value_type, int16_t, typename BASE_LEAF::config_type>(std::make_shared<window_shadow_block<int16_t, BASE_LEAF>>(base, shadow_begin_column.get_base_iter(), shadow_end_column.get_base_iter(), shadow_offsets, shadow_shape)); },
            }, shadow_type);
        }

        /**
         * Tuples iterator that wraps a column iterator.
         *
         * Tuples are computed by iterating rows within each column, then advancing the column iterator when one column
         * is exhausted.
         */
        class tuple_iterator: public base_input_iterator<tuple_iterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, typename window_shadow_block<IT, BASE_LEAF>::value_type>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            explicit tuple_iterator(const column_iterator &iter, const column_iterator &end) : iter(iter), end(end) {
                if (iter != end) {
                    col_ref = *iter;
                }
            }

            tuple_iterator(const tuple_iterator& rhs) : iter(rhs.iter), end(rhs.end), col_ref(rhs.col_ref) {}

            value_type operator*() {
                return value_type(*col_ref.rows_begin, col_ref.col, *col_ref.values_begin);
            }

            tuple_iterator& operator++() {
                // advance row within the column
                ++col_ref.rows_begin;
                ++col_ref.values_begin;

                if (col_ref.rows_begin == col_ref.rows_end) {
                    // this column exhausted, advance columns
                    ++iter;
                    if (iter != end) {
                        col_ref = *iter;
                    }
                }
                return *this;
            }

            bool operator==(const tuple_iterator& rhs) const {
                // same column and same row in the column
                return iter == rhs.iter && (iter == end || col_ref.rows_begin == col_ref.rows_begin);
            }

        private:
            column_iterator iter;
            const column_iterator end;
            typename column_iterator::value_type col_ref;
        };

        /**
         * @return a (begin, end) tuple of iterators that iterate over values in this block and return tuples.
         */
        range_t<tuple_iterator> tuples() const {
            return range_t<tuple_iterator>{
                    tuple_iterator(columns_begin(), columns_end()),
                    tuple_iterator(columns_end(), columns_end())
            };
        }

    protected:

        /**
         * @param iter column to check
         * @return true if there are any rows inside the window in the column pointed to by iter
         */
        bool are_rows_in_window(const typename BASE_LEAF::column_iterator& iter) const {
            auto ref = *iter;
            return ref.rows_begin != ref.rows_end &&
                   *ref.rows_begin <= row_inclusive_end &&
                   *(ref.rows_end - 1) >= row_begin;
        }

        /**
         * The block this is a window on
         */
        const std::shared_ptr<BASE_LEAF> base;

        const typename BASE_LEAF::column_iterator begin_column;
        const typename BASE_LEAF::column_iterator end_column;
        const column_iterator end_iter;

        const offset_t offsets;
        const typename BASE_LEAF::index_type row_begin;
        const typename BASE_LEAF::index_type row_inclusive_end;
    };
}

#endif //QUADMAT_WINDOW_SHADOW_BLOCK_H
