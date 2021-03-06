// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_WINDOW_SHADOW_BLOCK_H
#define QUADMAT_WINDOW_SHADOW_BLOCK_H

#include "quadmat/quadtree/block.h"
#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/util/base_iterators.h"
#include "quadmat/util/util.h"

namespace quadmat {

    /**
     *
     * @tparam IT
     * @tparam ShadowedLeaf
     */
    template<typename IT, typename ShadowedLeaf>
    class WindowShadowBlock: public Block<typename ShadowedLeaf::ValueType> {
    public:
        using ValueType = typename ShadowedLeaf::ValueType;
        using IndexType = IT;
        using ConfigType = typename ShadowedLeaf::ConfigType;
        using ShadowedLeafType = ShadowedLeaf;

        WindowShadowBlock(
                const std::shared_ptr<ShadowedLeaf> base,
                const typename ShadowedLeaf::ColumnIterator& begin_column,
                const typename ShadowedLeaf::ColumnIterator& end_column,
                const Offset &offsets,
                const Shape &shape)
                : shadowed_block_(base),
                  begin_column_(begin_column),
                  end_column_(end_column),
                  end_iter_(ColumnIterator(this, end_column, false)),
                  offsets_(offsets),
                  row_begin_(offsets.row_offset), row_inclusive_end_(offsets.row_offset - 1 + shape.nrows) {}

        /**
         * Reference to a single column
         */
        struct ColumnRef {
            IT col;
            OffsetIterator<typename ShadowedLeaf::RowIterator> rows_begin;
            OffsetIterator<typename ShadowedLeaf::RowIterator> rows_end;
            typename ShadowedLeaf::ValueIterator values_begin;
        };

        /**
         * Iterator type for iterating over this block's columns
         */
        class ColumnIterator: public BaseInputIterator<ColumnIterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = ColumnRef;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            /**
             *
             * @param shadow_block block this is an iterator on
             * @param iter column that is being shadowed
             * @param advance_ok If true and `iter` points at a column with no tuples in the window then will
             *                   auto-advance until a non-empty column (or the end) is found.
             *                   If false then the user is responsible to call AreRowsInWindow() before dereferencing
             */
            explicit ColumnIterator(const WindowShadowBlock<IT, ShadowedLeaf>* shadow_block, const typename ShadowedLeaf::ColumnIterator& iter, bool advance_ok = true) : shadow_block_(shadow_block), iter_(iter) {
                if (advance_ok) {
                    // It's possible that the given iter points at a column that doesn't have any rows that fit the window.
                    AdvanceToNextNonemptyCol();
                } // else it's a column point lookup and AreRowsInWindow() is called by the caller
            }
            ColumnIterator(const ColumnIterator& rhs) : shadow_block_(rhs.shadow_block_), iter_(rhs.iter_), cached_rows_begin_(rhs.cached_rows_begin_), cached_rows_end_(rhs.cached_rows_end_) {}

            value_type operator*() const {
                auto base_ref = *iter_;
                return {
                        .col = static_cast<IT>(base_ref.col - shadow_block_->offsets_.col_offset),
                        .rows_begin = OffsetIterator<typename ShadowedLeaf::RowIterator>(cached_rows_begin_, -shadow_block_->offsets_.row_offset),
                        .rows_end = OffsetIterator<typename ShadowedLeaf::RowIterator>(cached_rows_end_, -shadow_block_->offsets_.row_offset),
                        .values_begin = base_ref.values_begin + (cached_rows_begin_ - base_ref.rows_begin),
                };
            }

            ColumnIterator& operator++() {
                do {
                    ++iter_;
                } while (iter_ != shadow_block_->end_column_ && !AreRowsInWindow());
                return *this;
            }

            bool operator==(const ColumnIterator& rhs) const {
                return iter_ == rhs.iter_;
            }

            bool operator<(const ColumnIterator& rhs) const {
                return iter_ < rhs.iter_;
            }

            /**
             * @return iterator to the base column
             */
            const typename ShadowedLeaf::ColumnIterator& GetBaseIter() const {
                return iter_;
            }

            /**
             * @return true if there are any rows inside the window in the column pointed to by this
             */
            [[nodiscard]] bool AreRowsInWindow() {
                auto ref = *iter_;

                // a fast check to see if the row range falls outside the window
                if (ref.rows_begin == ref.rows_end ||
                       *ref.rows_begin > shadow_block_->row_inclusive_end_ ||
                       *(ref.rows_end - 1) < shadow_block_->row_begin_) {
                    return false;
                }

                // there might still not be any values in the window if the window is in an empty part of the column
                // however the column does now have valid iterators, so we can dereference and do a search
                cached_rows_begin_ = ref.rows_begin;
                cached_rows_end_ = ref.rows_end;
                TightenBounds(cached_rows_begin_, cached_rows_end_, shadow_block_->row_begin_, shadow_block_->row_inclusive_end_);

                return cached_rows_begin_ != cached_rows_end_;
            }

        private:
            void AdvanceToNextNonemptyCol() {
                while (iter_ != shadow_block_->end_column_ && !AreRowsInWindow()) {
                    ++iter_;
                }
            }
            const WindowShadowBlock<IT, ShadowedLeaf>* shadow_block_;
            typename ShadowedLeaf::ColumnIterator iter_;

            typename ShadowedLeaf::RowIterator cached_rows_begin_, cached_rows_end_;
        };

        /**
         * @return a column_iterator pointing at the first column
         */
        ColumnIterator ColumnsBegin() const {
            return ColumnIterator(this, begin_column_);
        }

        /**
         * @return a column_iterator pointing one past the last column
         */
        ColumnIterator ColumnsEnd() const {
            return end_iter_;
        }

        /**
         * @return a {begin, end} pair of iterators to iterate over all the columns of this block.
         */
        Range<ColumnIterator> GetColumns() const {
            return Range<ColumnIterator>{ColumnsBegin(), ColumnsEnd()};
        }

        /**
         * Reference to a single column, as looked up by a point lookup such as GetColumn()
         */
        struct PointLookupResult {
            typename ShadowedLeaf::PointLookupResult base_ref;
            const Index& row_offset;

            [[nodiscard]] bool IsColFound() const {
                return base_ref.IsColFound();
            }

            auto GetRowsBegin() {
                return NegativeOffsetReferenceIterator<typename ShadowedLeaf::RowIterator>(base_ref.GetRowsBegin(), row_offset);
            }

            auto GetRowsEnd() {
                return NegativeOffsetReferenceIterator<typename ShadowedLeaf::RowIterator>(base_ref.GetRowsEnd(), row_offset);
            }

            typename ShadowedLeaf::ValueIterator& GetValuesBegin() {
                return base_ref.GetValuesBegin();
            }
        };

        /**
         * Point lookup a column. Column may be empty.
         *
         * @param col column to look up
         * @return a PointLookupResult
         */
        PointLookupResult GetColumn(IT col) const noexcept {
            PointLookupResult ret{
                .base_ref = shadowed_block_->GetColumn(col + offsets_.col_offset),
                .row_offset = offsets_.row_offset,
            };

            if (!ret.base_ref.IsColFound()) {
                // column not in base block
                return ret;
            }

            // check if there are any tuples in the window

            // a fast check to see if the row range falls outside the window
            if (ret.base_ref.GetRowsBegin() == ret.base_ref.GetRowsEnd() ||
                *ret.base_ref.GetRowsBegin() > row_inclusive_end_ ||
                *(ret.base_ref.GetRowsEnd() - 1) < row_begin_) {

                ret.base_ref.col_found = false;
                return ret;
            }

            // Tighten the row iterators to the window
            auto rows_iter_increment = TightenBounds(ret.base_ref.rows_begin, ret.base_ref.rows_end, row_begin_, row_inclusive_end_);
            ret.base_ref.values_begin += rows_iter_increment;

            ret.base_ref.col_found = (ret.base_ref.rows_begin != ret.base_ref.rows_end);

            return ret;
        }

        /**
         * @param col column to look up. Use Index type because value could be larger than can fit in IT
         * @return column iterator point at col, or if there is no such column, the next larger column (or ColumnsEnd())
         */
        ColumnIterator GetColumnLowerBound(Index col) const {
            const typename ShadowedLeaf::ColumnIterator base_iter = shadowed_block_->GetColumnLowerBound(col + offsets_.col_offset);
            if (end_column_ <= base_iter) {
                return end_iter_;
            } else {
                return ColumnIterator(this, base_iter);
            }
        }

        /**
         * Warning: this method is O(n).
         *
         * @return number of tuples in this block.
         */
        [[nodiscard]] BlockNnn GetNnn() const {
            BlockNnn ret = 0;
            for (auto col : GetColumns()) {
                ret += col.rows_end - col.rows_begin;
            }
            return ret;
        }

        [[nodiscard]] BlockSizeInfo GetSize() const {
            return BlockSizeInfo{
                    .overhead_bytes = sizeof(WindowShadowBlock<IT, ShadowedLeaf>),
            };
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
        TreeNode<typename ShadowedLeaf::ValueType, typename ShadowedLeaf::ConfigType> GetShadowBlock(
            const std::shared_ptr<WindowShadowBlock<IT, ShadowedLeaf>>& ignored,
            const ColumnIterator& shadow_begin_column, const ColumnIterator& shadow_end_column,
            const Offset& shadow_offsets, const Shape& shadow_shape) {

            LeafIndex shadow_type = GetLeafIndexType(shadow_shape);

            return std::visit(overloaded{
                    [&](int64_t dim) -> TreeNode<typename ShadowedLeaf::ValueType, typename ShadowedLeaf::ConfigType> { return LeafCategory<typename ShadowedLeaf::ValueType, int64_t, typename ShadowedLeaf::ConfigType>(quadmat::allocate_shared<ConfigType, WindowShadowBlock<int64_t, ShadowedLeaf>>(shadowed_block_, shadow_begin_column.GetBaseIter(), shadow_end_column.GetBaseIter(), offsets_ + shadow_offsets, shadow_shape)); },
                    [&](int32_t dim) -> TreeNode<typename ShadowedLeaf::ValueType, typename ShadowedLeaf::ConfigType> { return LeafCategory<typename ShadowedLeaf::ValueType, int32_t, typename ShadowedLeaf::ConfigType>(quadmat::allocate_shared<ConfigType, WindowShadowBlock<int32_t, ShadowedLeaf>>(shadowed_block_, shadow_begin_column.GetBaseIter(), shadow_end_column.GetBaseIter(), offsets_ + shadow_offsets, shadow_shape)); },
                    [&](int16_t dim) -> TreeNode<typename ShadowedLeaf::ValueType, typename ShadowedLeaf::ConfigType> { return LeafCategory<typename ShadowedLeaf::ValueType, int16_t, typename ShadowedLeaf::ConfigType>(quadmat::allocate_shared<ConfigType, WindowShadowBlock<int16_t, ShadowedLeaf>>(shadowed_block_, shadow_begin_column.GetBaseIter(), shadow_end_column.GetBaseIter(), offsets_ + shadow_offsets, shadow_shape)); },
            }, shadow_type);
        }

        /**
         * Tuples iterator that wraps a column iterator.
         *
         * Tuples are computed by iterating rows within each column, then advancing the column iterator when one column
         * is exhausted.
         */
        class TupleIterator: public BaseInputIterator<TupleIterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, typename WindowShadowBlock<IT, ShadowedLeaf>::ValueType>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            explicit TupleIterator(const ColumnIterator &iter, const ColumnIterator &end) : iter_(iter), end_(end) {
                if (iter != end) {
                  col_ref_ = *iter;
                }
            }

            TupleIterator(const TupleIterator& rhs) : iter_(rhs.iter_), end_(rhs.end_), col_ref_(rhs.col_ref_) {}

            value_type operator*() {
                return value_type(*col_ref_.rows_begin, col_ref_.col, *col_ref_.values_begin);
            }

            TupleIterator& operator++() {
                // advance row within the column
                ++col_ref_.rows_begin;
                ++col_ref_.values_begin;

                if (col_ref_.rows_begin == col_ref_.rows_end) {
                    // this column exhausted, advance columns
                    ++iter_;
                    if (iter_ != end_) {
                      col_ref_ = *iter_;
                    }
                }
                return *this;
            }

            bool operator==(const TupleIterator& rhs) const {
                // same column and same row in the column
                return iter_ == rhs.iter_ && (iter_ == end_ || col_ref_.rows_begin == col_ref_.rows_begin);
            }

        private:
            ColumnIterator iter_;
            const ColumnIterator end_;
            typename ColumnIterator::value_type col_ref_;
        };

        /**
         * @return a (begin, end) tuple of iterators that iterate over values in this block and return tuples.
         */
        Range<TupleIterator> Tuples() const {
            return Range<TupleIterator>{
                TupleIterator(ColumnsBegin(), ColumnsEnd()),
                TupleIterator(ColumnsEnd(), ColumnsEnd())
            };
        }

    protected:

        /**
         * The block this is a window on
         */
        const std::shared_ptr<ShadowedLeaf> shadowed_block_;

        const typename ShadowedLeaf::ColumnIterator begin_column_;
        const typename ShadowedLeaf::ColumnIterator end_column_;
        const ColumnIterator end_iter_;

        const Offset offsets_;
        const typename ShadowedLeaf::IndexType row_begin_;
        const typename ShadowedLeaf::IndexType row_inclusive_end_;
    };
}

#endif //QUADMAT_WINDOW_SHADOW_BLOCK_H
