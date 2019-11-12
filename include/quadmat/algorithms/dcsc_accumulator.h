// Copyright (C) 2019 Adam Lugowski
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
     * @tparam CONFIG
     */
    template <typename T, typename IT, typename CONFIG = default_config>
    class dcsc_accumulator : public block<T> {
    public:
        explicit dcsc_accumulator(const shape_t &shape) : shape(shape) {}

        /**
         * Add a new block to this accumulator
         * @param new_block
         */
        void add(std::shared_ptr<dcsc_block<T, IT, CONFIG>> new_block) {
            children.push_back(new_block);
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
         * @tparam SR semiring, though only the add() is used to sum duplicate elements
         * @return a new DCSC Block that is the sum of all the blocks in this accumulator.
         */
        template <class SR = plus_times_semiring<T>>
        std::shared_ptr<dcsc_block<T, IT, CONFIG>> collapse(const SR& semiring = SR()) {
            return collapse_SpA<sparse_spa<IT, SR, CONFIG>, SR>(semiring);
        }

    protected:
        /**
         * Add up all the blocks in this accumulator using a sparse accumulator (SpA).
         *
         * @tparam SPA type of sparse accumulator to use
         * @tparam SR semiring, though only the add() is used to sum duplicate elements
         * @return a new DCSC Block that is the sum of all the blocks in this accumulator.
         */
        template <class SPA, class SR>
        std::shared_ptr<dcsc_block<T, IT, CONFIG>> collapse_SpA(const SR& semiring) {
            auto factory = dcsc_block_factory<T, IT, CONFIG>();

            std::priority_queue<column_ref, std::vector<column_ref, typename CONFIG::template TEMP_ALLOC<column_ref>>, std::greater<column_ref>> column_queue;

            // add the first columns of each child to the queue
            for (auto child : children) {
                column_ref cr{
                    .current = child->columns_begin(),
                    .end = child->columns_end()
                };

                if (cr.current != cr.end) {
                    column_queue.push(cr);
                }
            }

            // For each column i sum every column i from all children
            SPA spa(this->shape.nrows, semiring);
            while (!column_queue.empty()) {
                column_ref cr = column_queue.top();
                column_queue.pop();

                // fill the SpA with this column
                spa.update(cr.current.rows_begin(), cr.current.rows_end(), cr.current.values_begin());

                // if this is the last block with this column then dump the spa into the result
                if (column_queue.empty() || column_queue.top().col() != cr.col()) {
                    factory.add_spa(cr.col(), spa);
                    spa.clear();
                }

                ++cr.current;
                // if this block has more columns then put it back in the queue
                if (cr.current != cr.end) {
                    column_queue.push(cr);
                }
            }

            return factory.finish();
        }

        /**
         * Member of priority queue used to iterate over children's columns.
         */
        struct column_ref {
            using iter_type = typename dcsc_block<T, IT, CONFIG>::column_iterator;

            iter_type current;
            iter_type end;

            IT col() const {
                return *current;
            }

            bool operator>(const column_ref& rhs) const {
                return col() > rhs.col();
            }
        };
    protected:
        vector<std::shared_ptr<dcsc_block<T, IT, CONFIG>>> children;
        const shape_t &shape;
    };
}

#endif //QUADMAT_DCSC_ACCUMULATOR_H
