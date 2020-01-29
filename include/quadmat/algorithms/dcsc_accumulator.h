// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DCSC_ACCUMULATOR_H
#define QUADMAT_DCSC_ACCUMULATOR_H

#include <queue>

#include "quadmat/quadtree/block_container.h"
#include "quadmat/quadtree/leaf_blocks/dcsc_block.h"
#include "quadmat/algorithms/spa.h"

namespace quadmat {

    /**
     * A class that takes a list of dcsc_blocks and sums them up into a single one.
     *
     * @tparam T
     * @tparam IT
     * @tparam Config
     */
    template <typename T, typename IT, typename Config = DefaultConfig>
    class DcscAccumulator : public Block<T> {
    public:
        explicit DcscAccumulator(const Shape &shape) : shape_(shape) {}

        /**
         * Add a new block to this accumulator
         * @param new_block
         */
        void Add(std::shared_ptr<DcscBlock<T, IT, Config>> new_block) {
            children_.push_back(new_block);

#if QUADMAT_DEBUG_INLINE_SANITY_TESTING
            for (auto tup : new_block->Tuples()) {
                auto [row, col, value] = tup;

                // make sure tuple is within shape
                if (row >= shape_.nrows || col >= shape_.ncols) {
                    throw std::invalid_argument(Join::ToString("tuple <", row, ", ", col, ", ", value, "> outside of leaf shape ", shape.ToString()));
                }
            }
#endif
        }

        auto begin() const {
            return children_.begin();
        }

        auto end() const {
            return children_.end();
        }

        /**
         * Add up all the blocks in this accumulator.
         *
         * @tparam Semiring semiring, though only the Add() is used to sum duplicate elements
         * @return a new DCSC Block that is the sum of all the blocks in this accumulator.
         */
        template <class Semiring = PlusTimesSemiring<T>>
        std::shared_ptr<DcscBlock<T, IT, Config>> Collapse(const Semiring& semiring = Semiring()) {
            if (children_.size() == 1) {
                return children_.front();
            }

            int64_t max_estimated_flops = std::accumulate(std::begin(children_), std::end(children_), 0,
                [](const int64_t val, const auto& child) -> int64_t { return val + child->GetNnn(); });

            if (Config::template ShouldUseDenseSpa<T>(shape_.nrows, (double)max_estimated_flops)) {
                return CollapseUsingSpA<DenseSpa<IT, Semiring, Config>, Semiring>(semiring);
            } else {
                return CollapseUsingSpA<SparseSpa<IT, Semiring, Config>, Semiring>(semiring);
            }
        }

    protected:
        /**
         * Add up all the blocks in this accumulator using a sparse accumulator (SpA).
         *
         * @tparam Spa type of sparse accumulator to use
         * @tparam Semiring semiring, though only the Add() is used to sum duplicate elements
         * @return a new DCSC Block that is the sum of all the blocks in this accumulator.
         */
        template <class Spa, class Semiring>
        std::shared_ptr<DcscBlock<T, IT, Config>> CollapseUsingSpA(const Semiring& semiring) {
            auto factory = DcscBlockFactory<T, IT, Config>();

            std::priority_queue<ColumnRef, std::vector<ColumnRef, typename Config::template TempAllocator<ColumnRef>>, std::greater<ColumnRef>> column_queue;

            // add the first columns of each child to the queue
            for (auto child : children_) {
                ColumnRef cr{
                    .current = child->ColumnsBegin(),
                    .end = child->ColumnsEnd()
                };

                if (cr.current != cr.end) {
                    column_queue.push(cr);
                }
            }

            // For each column i sum every column i from all children
            Spa spa(this->shape_.nrows, semiring);
            while (!column_queue.empty()) {
                ColumnRef cr = column_queue.top();
                column_queue.pop();

                // fill the SpA with this column
                auto current_col = *cr.current;
                spa.Scatter(current_col.rows_begin, current_col.rows_end, current_col.values_begin);

                // if this is the last block with this column then dump the spa into the result
                if (column_queue.empty() || column_queue.top().col() != current_col.col) {
                    factory.AddColumnFromSpa(cr.col(), spa);
                    spa.Clear();
                }

                ++cr.current;
                // if this block has more columns then put it back in the queue
                if (cr.current != cr.end) {
                    column_queue.push(cr);
                }
            }

            return factory.Finish();
        }

        /**
         * Member of priority queue used to iterate over children's columns.
         */
        struct ColumnRef {
            using iter_type = typename DcscBlock<T, IT, Config>::ColumnIterator;

            iter_type current;
            iter_type end;

            IT col() const {
                return (*current).col;
            }

            bool operator>(const ColumnRef& rhs) const {
                return col() > rhs.col();
            }
        };
    protected:
        std::vector<std::shared_ptr<DcscBlock<T, IT, Config>>> children_;
        const Shape &shape_;
    };
}

#endif //QUADMAT_DCSC_ACCUMULATOR_H
