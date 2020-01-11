// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_TRIPLES_BLOCK_H
#define QUADMAT_TRIPLES_BLOCK_H

#include <numeric>
#include <utility>
#include <vector>

#include "quadmat/quadtree/block.h"
#include "quadmat/util/base_iterators.h"

namespace quadmat {

    /**
     * A simple block composed of <row, column, value> triples
     *
     * @tparam T value type, eg. double
     */
    template<typename T, typename IT, typename Config = DefaultConfig>
    class TriplesBlock: public Block<T> {
    public:
        using ValueType = T;
        using IndexType = IT;
        using ConfigType = Config;
        using PermutationVectorType = std::vector<size_t, typename Config::template TempAllocator<size_t>>;

        TriplesBlock() = default;

        [[nodiscard]] BlockSizeInfo GetSize() const {
            return BlockSizeInfo{
                rows_.size() * sizeof(IT) + cols_.size() * sizeof(IT),
                values_.size() * sizeof(T),
                sizeof(TriplesBlock<T, IT, Config>),
                values_.size()
            };
        }

        /**
         * Add a triple.
         *
         * @param row row index
         * @param col column index
         * @param value value
         */
        void Add(IT row, IT col, T value) {
            rows_.emplace_back(row);
            cols_.emplace_back(col);
            values_.emplace_back(value);
        }

        /**
         * Add many tuples
         *
         * @tparam Gen tuple generator template type
         * @param gen tuple generator
         */
        template <typename Gen>
        void Add(const Gen gen) {
            for (auto tup : gen) {
                rows_.emplace_back(std::get<0>(tup));
                cols_.emplace_back(std::get<1>(tup));
                values_.emplace_back(std::get<2>(tup));
            }
        }

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

            TupleIterator(const TriplesBlock<T, IT, Config>* block, size_t i) : BaseIndexedRandomAccessIterator<size_t, TupleIterator>(i), block_(block) {}
            TupleIterator(const TupleIterator& rhs) : BaseIndexedRandomAccessIterator<size_t, TupleIterator>(rhs.i_), block_(rhs.block_) {}

            value_type operator*() const {
                return std::tuple<IT, IT, T>(block_->rows_[this->i_], block_->cols_[this->i_], block_->values_[this->i_]);
            }

        private:
            const TriplesBlock<T, IT, Config>* block_;
        };

        /**
         * Input Iterator type for iterating over this block's tuples in permuted order
         */
        class PermutedIterator: public BaseIndexedRandomAccessIterator<size_t, PermutedIterator> {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = std::tuple<IT, IT, T>;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;

            explicit PermutedIterator(
                    const TriplesBlock<T, IT, Config>* block,
                    std::shared_ptr<PermutationVectorType> permutation,
                    std::size_t i)
                    : BaseIndexedRandomAccessIterator<size_t, PermutedIterator>(i),
                      permutation_(std::move(permutation)),
                      block_(block) {}
            PermutedIterator(
                    const PermutedIterator& rhs)
                    : BaseIndexedRandomAccessIterator<size_t, PermutedIterator>(rhs.i_),
                      permutation_(rhs.permutation_),
                      block_(rhs.block_) {}

            value_type operator*() const {
                return std::tuple<IT, IT, T>(
                    block_->rows_[(*permutation_)[this->i_]],
                    block_->cols_[(*permutation_)[this->i_]],
                    block_->values_[(*permutation_)[this->i_]]);
            }

        private:
            std::shared_ptr<PermutationVectorType> permutation_;
            const TriplesBlock<T, IT, Config>* block_;
        };

        /**
         * @return a (begin, end) pair of tuple iterators that return tuples in order sorted by column, row.
         */
        Range<TupleIterator> OriginalTuples() const {
            return Range<TupleIterator>{
                TupleIterator(this, 0),
                TupleIterator(this, rows_.size())
            };
        }

        /**
         * @return a (begin, end) pair of tuple iterators that return tuples in order sorted by column, row.
         */
        Range<PermutedIterator> SortedTuples() const {
            auto permutation = GetSortPermutation();
            return Range<PermutedIterator>{
                PermutedIterator(this, permutation, 0),
                PermutedIterator(this, permutation, rows_.size())
            };
        }

        /**
         *
         * @param permutation a permutation vector
         * @param p_begin start of permutation range
         * @param p_end end of permutation range
         * @return a (begin, end) pair of tuple iterators that return tuples in the order specified by permutation.
         */
        Range<PermutedIterator> PermutedTuples(std::shared_ptr<PermutationVectorType> permutation,
                                               typename PermutationVectorType::iterator p_begin,
                                               typename PermutationVectorType::iterator p_end) const {
            return Range<PermutedIterator>{
                PermutedIterator(this, permutation, p_begin - permutation->begin()),
                PermutedIterator(this, permutation, p_end - permutation->begin())
            };
        }

        /**
         * @return number of tuples in this block.
         */
        [[nodiscard]] BlockNnn GetNnn() const {
            return values_.size();
        }

        /**
         * @param i
         * @return the row index of tuple i
         */
        [[nodiscard]] IT GetRow(size_t i) const {
            return rows_[i];
        }

        /**
         * @param i
         * @return the column index of tuple i
         */
        [[nodiscard]] IT GetCol(size_t i) const {
            return cols_[i];
        }

    protected:
        std::vector<IT, typename Config::template Allocator<IT>> rows_;
        std::vector<IT, typename Config::template Allocator<IT>> cols_;
        std::vector<T, typename Config::template Allocator<T>> values_;

        /**
         * Get a permutation that would order the triples by column then row.
         */
        std::shared_ptr<PermutationVectorType> GetSortPermutation() const
        {
            auto p = std::make_shared<PermutationVectorType>(rows_.size());
            std::iota(p->begin(), p->end(), 0);
            std::sort(p->begin(), p->end(),
                      [&](size_t i, size_t j) {
                if (cols_[i] != cols_[j]) {
                    return cols_[i] < cols_[j];
                } else if (rows_[i] != rows_[j]) {
                    return rows_[i] < rows_[j];
                } else {
                    return i < j;
                }
            });
            return p;
        }
    };
}

#endif //QUADMAT_TRIPLES_BLOCK_H
