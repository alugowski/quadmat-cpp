// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DCSC_BLOCK_H
#define QUADMAT_DCSC_BLOCK_H

#include <algorithm>
#include <numeric>
#include <vector>

#include "quadmat/quadtree/tree_nodes.h"
#include "quadmat/util/base_iterators.h"
#include "quadmat/util/util.h"
#include "quadmat/quadtree/leaf_blocks/window_shadow_block.h"
#include "quadmat/quadtree/tree_visitors.h"

namespace quadmat {

    /**
     * A Doubly-Compressed Sparse Columns block.
     *
     * This format is similar to CSC, but the column array is again compressed to not allow empty columns.
     * Only columns with at least a single non-null are represented.
     *
     * @tparam T value type
     * @tparam IT index type
     * @tparam Config storage configuration options
     */
    template<typename T, typename IT, typename Config>
    class DcscBlock: public Block<T> {
    public:
        using ValueType = T;
        using IndexType = IT;
        using ConfigType = Config;

        using RowIterator = typename std::vector<IT, typename ConfigType::template Allocator<IT>>::const_iterator;
        using ValueIterator = typename std::vector<ValueType, typename ConfigType::template Allocator<ValueType>>::const_iterator;

        DcscBlock() = default;
    public:
        /**
         * Create a DCSC block. Use DcscBlockFactory to create these vectors.
         *
         * @param col_ind
         * @param col_ptr
         * @param row_ind
         * @param values
         */
        DcscBlock(std::vector<IT, typename Config::template Allocator<IT>>&& col_ind,
                  std::vector<BlockNnn, typename Config::template Allocator<BlockNnn>>&& col_ptr,
                  std::vector<IT, typename Config::template Allocator<IT>>&& row_ind,
                  std::vector<T, typename Config::template Allocator<T>>&& values,
                  std::vector<bool, typename Config::template Allocator<bool>>&& col_ind_mask,
                  std::vector<BlockNnn, typename Config::template Allocator<BlockNnn>>&& csc_col_ptr) :
            col_ind_(std::move(col_ind)),
            col_ptr_(std::move(col_ptr)),
            row_ind_(std::move(row_ind)),
            values_(std::move(values)),
            col_ind_mask_(std::move(col_ind_mask)),
            csc_col_ptr_(std::move(csc_col_ptr)) {}

        /**
         * Iterator type for iterating over this block's tuples
         */
        class TupleIterator: public BaseIndexedRandomAccessIterator<size_t, TupleIterator> {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            TupleIterator(const DcscBlock<T, IT, Config>* block, size_t i, IT col_idx) : BaseIndexedRandomAccessIterator<size_t, TupleIterator>(i), block_(block), col_idx_(col_idx) {}
            TupleIterator(const TupleIterator& rhs) : BaseIndexedRandomAccessIterator<size_t, TupleIterator>(rhs.i_), block_(rhs.block_), col_idx_(rhs.col_idx_) {}

            value_type operator*() {
                return std::tuple<IT, IT, T>(block_->row_ind_[this->i_], block_->col_ind_[col_idx_], block_->values_[this->i_]);
            }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
            TupleIterator& operator++() {
                ++this->i_;
                if (this->i_ == block_->col_ptr_[col_idx_ + 1]) {
                    // new column
                    ++col_idx_;
                }
                return *this;
            }

            TupleIterator& operator+=(std::ptrdiff_t n) {
                this->i_ += n;
                // find which column we are now in
                auto col_greater = std::upper_bound(begin(block_->col_ptr_), end(block_->col_ptr_), this->i_);
              col_idx_ = (col_greater - begin(block_->col_ptr_)) - 1;
                return *this;
            }
#pragma clang diagnostic pop

        private:
            const DcscBlock<T, IT, Config>* block_;
            /**
             * Index of current column
             */
            IT col_idx_;
        };

        /**
         * @return a (begin, end) tuple of iterators that iterate over values in this block and return tuples.
         */
        Range<TupleIterator> Tuples() const {
            return Range<TupleIterator>{
                TupleIterator(this, 0, 0),
                TupleIterator(this, row_ind_.size(), col_ptr_.size())
            };
        }

        /**
         * Reference to a single column
         */
        struct ColumnRef {
            IT col;
            RowIterator rows_begin;
            RowIterator rows_end;
            ValueIterator values_begin;
        };

        /**
         * Iterator type for iterating over this block's columns
         */
        class ColumnIterator: public BaseIndexedRandomAccessIterator<BlockNnn, ColumnIterator> {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = ColumnRef;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            ColumnIterator(const DcscBlock<T, IT, Config>* block, BlockNnn i) : BaseIndexedRandomAccessIterator<BlockNnn, ColumnIterator>(i), block_(block) {}
            ColumnIterator(const ColumnIterator& rhs) : BaseIndexedRandomAccessIterator<BlockNnn, ColumnIterator>(rhs.i_), block_(rhs.block_) {}

            value_type operator*() const noexcept {
                return {
                    .col = block_->col_ind_[this->i_],
                    .rows_begin = block_->row_ind_.cbegin() + block_->col_ptr_[this->i_],
                    .rows_end = block_->row_ind_.cbegin() + block_->col_ptr_[this->i_ + 1],
                    .values_begin = block_->values_.cbegin() + block_->col_ptr_[this->i_],
                };
            }

        private:
            const DcscBlock<T, IT, Config>* block_;
        };

        /**
         * @return a column_iterator pointing at the first column
         */
        ColumnIterator ColumnsBegin() const {
            return ColumnIterator(this, 0);
        }

        /**
         * @return a column_iterator pointing one past the last column
         */
        ColumnIterator ColumnsEnd() const {
            return ColumnIterator(this, col_ind_.size());
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
            bool col_found;
            RowIterator rows_begin; // only set if IsColFound() == true
            RowIterator rows_end; // only set if IsColFound() == true
            ValueIterator values_begin; // only set if IsColFound() == true

            [[nodiscard]] bool IsColFound() const {
                return col_found;
            }

            RowIterator& GetRowsBegin() {
                return rows_begin;
            }

            RowIterator& GetRowsEnd() {
                return rows_end;
            }

            ValueIterator& GetValuesBegin() {
                return values_begin;
            }
        };

        /**
         * Point lookup a column. Column may be empty.
         *
         * @param col column to look up
         * @return a PointLookupResult
         */
        PointLookupResult GetColumn(IT col) const noexcept {
            if (!csc_col_ptr_.empty()) {
                // using CSC index
                bool in_range = col < (csc_col_ptr_.size() - 1);
                auto first = in_range ? csc_col_ptr_[col] : 0;
                auto last = in_range ? csc_col_ptr_[col + 1] : 0;
                return {
                    .col_found = first != last,
                    .rows_begin = row_ind_.cbegin() + first,
                    .rows_end = row_ind_.cbegin() + last,
                    .values_begin = values_.cbegin() + first,
                };
            }

            if (!col_ind_mask_.empty()) {
                // using mask optimization
                if (col >= col_ind_mask_.size() || !col_ind_mask_[col]) {
                    return {
                        .col_found = false,
                    };
                }
            }

            auto pos = std::lower_bound(begin(col_ind_), end(col_ind_), col);
            if (pos == end(col_ind_) || *pos != col) {
                return {
                    .col_found = false,
                };
            }

            auto ptr_idx = pos - begin(col_ind_);

            return {
                .col_found = true,
                .rows_begin = row_ind_.cbegin() + col_ptr_[ptr_idx],
                .rows_end = row_ind_.cbegin() + col_ptr_[ptr_idx + 1],
                .values_begin = values_.cbegin() + col_ptr_[ptr_idx],
            };
        }

        /**
         * @param col column to look up
         * @return column iterator point at col, or if there is no such column, the next larger column (or ColumnsEnd())
         */
        ColumnIterator GetColumnLowerBound(Index col) const {
            if (col_ind_.empty() || col > col_ind_.back()) {
                // Also handle cases where the column is larger than can fit in IT.
                return ColumnsEnd();
            }

            auto pos = std::lower_bound(begin(col_ind_), end(col_ind_), static_cast<IT>(col));
            return ColumnIterator(this, pos - begin(col_ind_));
        }

        [[nodiscard]] BlockSizeInfo GetSize() const {
            return BlockSizeInfo{
                col_ind_.size() * sizeof(IT) + col_ptr_.size() * sizeof(BlockNnn) + row_ind_.size() * sizeof(IT) + col_ind_mask_.size() / 8,
                values_.size() * sizeof(T),
                sizeof(DcscBlock<T, IT, Config>),
                values_.size()
            };
        }

        /**
         * @return number of tuples in this block.
         */
        [[nodiscard]] BlockNnn GetNnn() const {
            return values_.size();
        }

        /**
         * Create a shadow block that provides a view of a part of this leaf block
         */
        static LeafNode<T, Config> GetShadowBlock(const std::shared_ptr<DcscBlock<T, IT, Config>> base_dcsc, const Offset& offsets, const Shape& shape) {
            // find the column range for the shadow
            auto begin_column_pos = std::lower_bound(begin(base_dcsc->col_ind_), end(base_dcsc->col_ind_), offsets.col_offset);
            auto begin_column = ColumnIterator(base_dcsc.get(), begin_column_pos - begin(base_dcsc->col_ind_));

            auto end_column_pos = std::upper_bound(begin(base_dcsc->col_ind_), end(base_dcsc->col_ind_), offsets.col_offset - 1 + shape.ncols);
            auto end_column = ColumnIterator(base_dcsc.get(), end_column_pos - begin(base_dcsc->col_ind_));

            LeafIndex shadow_type = GetLeafIndexType(shape);

            return std::visit(overloaded{
                    [&](int64_t dim) -> LeafNode<T, Config> { return LeafCategory<T, int64_t, Config>(quadmat::allocate_shared<Config, WindowShadowBlock<int64_t, DcscBlock<T, IT, Config>>>(base_dcsc, begin_column, end_column, offsets, shape)); },
                    [&](int32_t dim) -> LeafNode<T, Config> { return LeafCategory<T, int32_t, Config>(quadmat::allocate_shared<Config, WindowShadowBlock<int32_t, DcscBlock<T, IT, Config>>>(base_dcsc, begin_column, end_column, offsets, shape)); },
                    [&](int16_t dim) -> LeafNode<T, Config> { return LeafCategory<T, int16_t, Config>(quadmat::allocate_shared<Config, WindowShadowBlock<int16_t, DcscBlock<T, IT, Config>>>(base_dcsc, begin_column, end_column, offsets, shape)); },
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
        LeafNode<T, Config> GetShadowBlock(const std::shared_ptr<DcscBlock<T, IT, Config>> base_dcsc,
                                           const ColumnIterator& shadow_begin_column, const ColumnIterator& shadow_end_column,
                                           const Offset& shadow_offsets, const Shape& shadow_shape) {
            LeafIndex shadow_type = GetLeafIndexType(shadow_shape);

            return std::visit(overloaded{
                    [&](int64_t dim) -> LeafNode<T, Config> { return LeafCategory<T, int64_t, Config>(quadmat::allocate_shared<Config, WindowShadowBlock<int64_t, DcscBlock<T, IT, Config>>>(base_dcsc, shadow_begin_column, shadow_end_column, shadow_offsets, shadow_shape)); },
                    [&](int32_t dim) -> LeafNode<T, Config> { return LeafCategory<T, int32_t, Config>(quadmat::allocate_shared<Config, WindowShadowBlock<int32_t, DcscBlock<T, IT, Config>>>(base_dcsc, shadow_begin_column, shadow_end_column, shadow_offsets, shadow_shape)); },
                    [&](int16_t dim) -> LeafNode<T, Config> { return LeafCategory<T, int16_t, Config>(quadmat::allocate_shared<Config, WindowShadowBlock<int16_t, DcscBlock<T, IT, Config>>>(base_dcsc, shadow_begin_column, shadow_end_column, shadow_offsets, shadow_shape)); },
            }, shadow_type);
        }

    protected:
        const std::vector<IT, typename Config::template Allocator<IT>> col_ind_;
        const std::vector<BlockNnn, typename Config::template Allocator<BlockNnn>> col_ptr_;
        const std::vector<IT, typename Config::template Allocator<IT>> row_ind_;
        const std::vector<T, typename Config::template Allocator<T>> values_;

        /**
         * An optional bitfield for column existence testing. Speeds up GetColumn().
         */
        const std::vector<bool, typename Config::template Allocator<bool>> col_ind_mask_;

        /**
         * An optional CSC column pointer array. Speeds up GetColumn().
         */
        const std::vector<BlockNnn, typename Config::template Allocator<BlockNnn>> csc_col_ptr_;
    };

    /**
     * A tool to construct a DCSC block column by column. DCSC blocks are immutable after construction.
     *
     * @tparam T
     * @tparam IT
     * @tparam Config
     */
    template<typename T, typename IT, typename Config = DefaultConfig>
    class DcscBlockFactory {
    public:
        DcscBlockFactory() = default;

        /**
         * Create a DCSC block from (column, row)-ordered tuples. Single pass.
         *
         * @tparam Gen tuple generator template type
         * @param ncols number of columns in this block
         * @param nnn estimated number of nonzeros in this block.
         * @param col_ordered_gen tuple generator. **Must return tuples ordered by column, row**
         */
        template <typename Gen>
        DcscBlockFactory(const BlockNnn nnn, const Gen col_ordered_gen) {
            // reserve memory
            row_ind_.reserve(nnn);
            values_.reserve(nnn);

            IT prev_col = -1;
            BlockNnn i = 0;
            for (auto tup : col_ordered_gen) {
                IT col = std::get<1>(tup);
                if (prev_col != col) {
                    // new column
                    col_ind_.emplace_back(col);
                    col_ptr_.emplace_back(i);
                }
                prev_col = col;

                row_ind_.emplace_back(std::get<0>(tup)); // add row
                values_.emplace_back(std::get<2>(tup)); // add value
                i++;
            }
        }

        /**
         * Dump the contents of a SpA as the last column of the DcscBlock.
         *
         * @param col column to write the spa as. Must be greater than any column added so far.
         * @param spa
         */
        template <class Spa>
        void AddColumnFromSpa(IT col, Spa& spa) {
            // col > prev_col

            if (spa.IsEmpty()) {
                return;
            }

            col_ind_.emplace_back(col);
            col_ptr_.emplace_back(row_ind_.size());

            spa.Gather(row_ind_, values_);
        }

        /**
         * Call to return the constructed block. Also caps the data structures.
         *
         * No further calls to any methods in this class allowed after calling this method.
         *
         * @return a unique_ptr to the constructed DcscBlock
         */
        auto Finish() {
            // cap off the columns
            col_ptr_.emplace_back(row_ind_.size());

            // save memory
            col_ind_.shrink_to_fit();
            col_ptr_.shrink_to_fit();
            row_ind_.shrink_to_fit();
            values_.shrink_to_fit();

            // create a CSC index, if appropriate
            std::vector<BlockNnn, typename Config::template Allocator<BlockNnn>> csc_col_ptr;

            Index ncols = col_ind_.empty() ? 0 : col_ind_.back() + 1;
            if (ncols > 0 && Config::ShouldUseCscIndex(ncols, col_ind_.size())) {
                csc_col_ptr.resize(ncols + 1);

                // We have at least one column. Break out the first iteration so that
                // each loop iteration can be independent.
                std::fill(&csc_col_ptr[0], &csc_col_ptr[1], col_ptr_[0]);
                for (int i = 1; i < col_ind_.size(); ++i) {
                    BlockNnn prev_col = col_ind_[i - 1];
                    BlockNnn col = col_ind_[i];
                    std::fill(&csc_col_ptr[prev_col + 1], &csc_col_ptr[col + 1], col_ptr_[i]);
                }

                csc_col_ptr.back() = col_ptr_.back();
            }

            // create a lookup mask, if appropriate
            std::vector<bool, typename Config::template Allocator<bool>> col_ind_mask_;

            if (csc_col_ptr.empty() && ncols > 0 && Config::ShouldUseDcscBoolMask(ncols, col_ind_.size())) {
                col_ind_mask_.resize(ncols);

                // mark non-empty columns in the mask
                for (const auto& col : col_ind_) {
                    col_ind_mask_[col] = true;
                }
            }

            // construction complete
            return quadmat::allocate_unique<Config, DcscBlock<T, IT, Config>>(
                    std::move(col_ind_),
                    std::move(col_ptr_),
                    std::move(row_ind_),
                    std::move(values_),
                    std::move(col_ind_mask_),
                    std::move(csc_col_ptr)
                    );
        }

        /**
         * Alternative to Finish() but returns a shared_ptr.
         * @return
         */
        std::shared_ptr<DcscBlock<T, IT, Config>> FinishShared() {
            return ToShared(std::move(Finish()));
        }

    protected:
        std::vector<IT, typename Config::template Allocator<IT>> col_ind_;
        std::vector<BlockNnn, typename Config::template Allocator<BlockNnn>> col_ptr_;
        std::vector<IT, typename Config::template Allocator<IT>> row_ind_;
        std::vector<T, typename Config::template Allocator<T>> values_;
    };
}

#endif //QUADMAT_DCSC_BLOCK_H
