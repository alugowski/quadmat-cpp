// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_DCSC_ACCUMULATOR_H
#define QUADMAT_DCSC_ACCUMULATOR_H

#include <queue>

#include "block_container.h"
#include "dcsc_block.h"
#include "spa.h"

namespace quadmat {

    /**
     * A class that takes a list of dcsc_blocks and sums them up into a single one.
     *
     * @tparam T
     * @tparam IT
     * @tparam CONFIG default value in forward declaration in dcsc_block.h
     */
    template <typename T, typename IT, typename CONFIG>
    class dcsc_accumulator : public block<T>, public block_container<T, CONFIG> {
    public:
        explicit dcsc_accumulator(const shape_t &shape) : block<T>(shape) {}

        /**
         * Add a new block to this accumulator
         * @param new_block
         */
        void add(std::shared_ptr<dcsc_block<T, IT, CONFIG>> new_block) {
            assert(new_block->get_shape() == this->shape);

            children.push_back(new_block);
        }

        [[nodiscard]] size_t num_children() const override {
            return children.size();
        }

        auto begin() const {
            return children.begin();
        }

        auto end() const {
            return children.end();
        }

        /**
         * Add up all the blocks in this accumulator.
         *
         * @tparam ADDER how to sum duplicate elements
         * @return a new DCSC Block that is the sum of all the blocks in this accumulator.
         */
        template <class ADDER = std::plus<T>>
        std::shared_ptr<dcsc_block<T, IT, CONFIG>> collapse(const ADDER& adder = ADDER()) {
            return collapse_SpA<sparse_spa<T, IT, ADDER, CONFIG>, ADDER>(adder);
        }

    protected:
        /**
         * Add up all the blocks in this accumulator using a sparse accumulator (SpA).
         *
         * @tparam SPA type of sparse accumulator to use
         * @tparam ADDER how to sum duplicate elements
         * @return a new DCSC Block that is the sum of all the blocks in this accumulator.
         */
        template <class SPA, class ADDER>
        std::shared_ptr<dcsc_block<T, IT, CONFIG>> collapse_SpA(const ADDER& adder) {
            auto ret = std::make_shared<dcsc_block<T, IT, CONFIG>>(this->shape);

            std::priority_queue<column_ref, std::vector<column_ref, typename CONFIG::template TEMP_ALLOC<column_ref>>, std::greater<column_ref>> column_queue;
            SPA spa(this->shape.nrows, adder);

            // add all the first columns to the queue
            for (auto child : children) {
                if (child->row_ind.size() == 0) {
                    // empty child, ignore it
                    continue;
                }
                column_queue.push(column_ref{
                    .block = child.get(),
                    .col = child->col_ind[0],
                    .last_col = child->col_ind[child->col_ind.size()-1],
                    .col_idx = 0
                });
            }

            // go column by column and collapse each column at a time
            IT prev_col = -1;
            while (!column_queue.empty()) {
                column_ref cr = column_queue.top();
                column_queue.pop();

                // copy the SpA into the result block
                if (cr.col != prev_col && prev_col != -1) {
                    dump_spa(prev_col, spa, ret.get());
                    spa.clear();
                }
                prev_col = cr.col;

                // fill the SpA with this column
                auto rows_iter = cr.block->rows_begin(cr.col_idx);
                auto rows_end = cr.block->rows_end(cr.col_idx);
                auto values_iter = cr.block->values_begin(cr.col_idx);

                while (rows_iter != rows_end) {
                    spa.update(*rows_iter, *values_iter);

                    ++rows_iter;
                    ++values_iter;
                }

                // if this block has more columns then put it back in the queue
                if (cr.col < cr.last_col) {
                    cr.col_idx++;
                    cr.col = cr.block->col_ind[cr.col_idx];
                    column_queue.push(cr);
                }
            }

            // cap off the columns by repeating the last value
            if (prev_col != -1) {
                dump_spa(prev_col, spa, ret.get());

                ret->col_ind.emplace_back(ret->col_ind[ret->col_ind.size() - 1]);
                ret->col_ptr.emplace_back(ret->row_ind.size());
            }

            return ret;
        }

        /**
         * Dump the contents of a SpA into the end of a dcsc_block.
         */
        template <class SPA>
        void dump_spa(IT col, SPA& spa, dcsc_block<T, IT, CONFIG>* ret) {
            ret->col_ind.emplace_back(col);
            ret->col_ptr.emplace_back(ret->row_ind.size());

            for (auto it : spa) {
                ret->row_ind.emplace_back(it.first);
                ret->values.emplace_back(it.second);
            }
        }

        /**
         * Member of priority queue used to iterate over children's columns.
         */
        struct column_ref {
            dcsc_block<T, IT, CONFIG>* block;
            IT col;
            IT last_col;
            blocknnn_t col_idx;

            bool operator>(const column_ref& rhs) const {
                return col > rhs.col;
            }
        };
    protected:
        vector<std::shared_ptr<dcsc_block<T, IT, CONFIG>>> children;
    };
}

#endif //QUADMAT_DCSC_ACCUMULATOR_H
