// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MULTIPLY_TREES_H
#define QUADMAT_MULTIPLY_TREES_H

#include <type_traits>
#include "tree_visitors.h"
#include "algorithms/multiply_leaves.h"

namespace quadmat {

    enum pair_status_t {
        EMPTY_PAIR = 0,

        /**
         * One or both of the nodes are empty. Pair can be ignored.
         */
        HAS_EMPTY = 1,

        /**
         * One or both of the nodes are a future block. Computation must wait until the future is computed.
         */
        HAS_FUTURE = 1u << 1u,

        /**
         * One or both of the nodes are an inner block. Recurse, possibly with a shadow block.
         */
        HAS_INNER = 1u << 2u,

        /**
         * One or both of the nodes are a leaf block. If there is an inner block as well then a shadow subdivision is required.
         */
        HAS_LEAF = 1u << 3u,

        HAS_LEAF_16 = 1u << 4u,
        HAS_LEAF_32 = 1u << 5u,
        HAS_LEAF_64 = 1u << 6u,

        /**
         * Has two leaves of different index sizes. Error condition that should never occur.
         */
        HAS_MISMATCHED_LEAVES = 1u << 7u
    };

    /**
     * A binding of two blocks that will be multiplied together.
     *
     * @tparam LT block A's type
     * @tparam RT block B's type
     * @tparam CONFIG configuration shared by both blocks
     */
    template <typename LT, typename RT, typename CONFIG>
    struct tree_node_pair_t {
        tree_node_t<LT, CONFIG> a;
        tree_node_t<RT, CONFIG> b;

        tree_node_pair_t(const tree_node_t<LT, CONFIG> &a, const tree_node_t<RT, CONFIG> &b) : a(a), b(b) {}

        /**
         * Visitor that describes the contents of this tree_node_pair.
         */
        template <typename T>
        struct status_visitor_t {
            unsigned operator()(const std::monostate& ignored) {
                return HAS_EMPTY;
            }

            unsigned operator()(const std::shared_ptr<future_block<T, CONFIG>>& fb) {
                return HAS_FUTURE;
            }

            unsigned operator()(const std::shared_ptr<inner_block<T, CONFIG>>& inner) {
                return HAS_INNER;
            }

            unsigned operator()(const leaf_category_t<T, int64_t, CONFIG> &leaf) {
                return HAS_LEAF | HAS_LEAF_64;
            }

            unsigned operator()(const leaf_category_t<T, int32_t, CONFIG> &leaf) {
                return HAS_LEAF | HAS_LEAF_32;
            }

            unsigned operator()(const leaf_category_t<T, int16_t, CONFIG> &leaf) {
                return HAS_LEAF | HAS_LEAF_16;
            }
        };

        /**
         * Get a summary of what is inside this pair.
         *
         * @return a bitwise OR of status_t enum states
         */
        [[nodiscard]] unsigned get_status() const {
            unsigned a_status = std::visit(status_visitor_t<LT>{}, a);
            unsigned b_status = std::visit(status_visitor_t<RT>{}, b);

            // if both are leaves make sure they are of the same size
            if ((a_status & HAS_LEAF) == HAS_LEAF &&
                    (b_status & HAS_LEAF) == HAS_LEAF) {
                if (a_status != b_status) {
                    return HAS_MISMATCHED_LEAVES;
                }
            }

            return a_status | b_status;
        }
    };

    /**
     * A set of block pairs that when multiplied then summed will result in the output block.
     *
     * @tparam LT side a's type
     * @tparam RT side b's type
     * @tparam CONFIG configuration shared by both sides
     */
    template <typename LT, typename RT, typename CONFIG>
    struct pair_set_t {
        vector<tree_node_pair_t<LT, RT, CONFIG>, typename CONFIG::template TEMP_ALLOC<tree_node_pair_t<LT, RT, CONFIG>>> pairs;

        pair_set_t() = default;
        explicit pair_set_t(const tree_node_t<LT, CONFIG>& a, const tree_node_t<LT, CONFIG>& b) : pairs{{a, b}} {}

        /**
         * Prune any pair in the pair set that has an empty block. The result of that multiplication is also an empty block.
         *
         * @return the bitwise OR of all statuses of remaining pairs, or 0 if empty.
         */
        unsigned prune_empty() {
            unsigned ret = 0;
            for (auto cur = pairs.begin(); cur != pairs.end();) {
                unsigned pair_status = cur->get_status();
                if ((pair_status & HAS_EMPTY) == HAS_EMPTY) {
                    pairs.erase(cur);
                } else {
                    ret |= pair_status;
                    ++cur;
                }
            }

            // test for mismatches
            unsigned constexpr leaf_bit_mask = (unsigned)HAS_LEAF_64 | HAS_LEAF_32 | HAS_LEAF_16;
            if (ret & leaf_bit_mask) {
                unsigned b = ret & leaf_bit_mask;
                if (!(b && !(b & (b-1)))) {
                    // more than one bit set, so there are leaf blocks of different sizes present
                    ret |= HAS_MISMATCHED_LEAVES;
                }
            }

            return ret;
        }
    };

    /**
     * Constructs and organizes the tasks necessary to multiply two quadmat trees.
     *
     * @tparam SR semiring to use
     * @tparam CONFIG configuration
     */
    template <class SR, typename CONFIG = basic_settings>
    class spawn_multiply_job {
    public:
        using LT = typename SR::map_type_l;
        using RT = typename SR::map_type_r;
        using RETT = typename SR::reduce_type;

        spawn_multiply_job(
                const pair_set_t<LT, RT, CONFIG> &pair_set,
                std::shared_ptr<block_container<RETT, CONFIG>> dest_bc,
                int dest_position,
                const offset_t &dest_offsets,
                const shape_t& dest_shape,
                const SR& semiring = SR())
                : pair_set(pair_set), dest_bc(dest_bc), dest_position(dest_position), dest_offsets(dest_offsets), dest_shape(dest_shape), semiring(semiring) {
        }

        /**
         * Run multiply job.
         */
        bool run() {
            // see what needs to be done
            unsigned status = pair_set.prune_empty();

            // see if pair set has anything to do
            if (status == EMPTY_PAIR) {
                // empty
                return true;
            }

            // sanity check
            if ((status & HAS_MISMATCHED_LEAVES) == HAS_MISMATCHED_LEAVES) {
                throw node_type_mismatch();
            }

            // check if there are any future blocks then we must wait for them
            if ((status & HAS_FUTURE) == HAS_FUTURE) {
                throw not_implemented("waiting on future blocks");
            }

            // check if there are inner blocks
            if ((status & HAS_INNER) == HAS_INNER) {
                return recurse();
            }

            // only leaves remaining, do multiplication
            return multiply_leaves(status);
        }

    protected:

        /**
         * pair_set contains an inner block. Put an inner_block at the destination, build pair sets for each one,
         * and recurse.
         */
        bool recurse() {
            // construct result
            std::shared_ptr<inner_block<RETT, CONFIG>> recursive_dest_block = dest_bc->create_inner(dest_position);

            // build recursive pair sets
            array<pair_set_t<LT, RT, CONFIG>, 4> recursive_pair_sets{};

            for (auto pair : pair_set.pairs) {
                std::visit(recursive_visitor_t(recursive_pair_sets), pair.a, pair.b);
            }

            // recurse
            for (auto recursive_child_pos : all_inner_positions) {
                spawn_multiply_job job(
                        recursive_pair_sets[recursive_child_pos],
                        recursive_dest_block,
                        recursive_child_pos,
                        recursive_dest_block->get_offsets(recursive_child_pos, dest_offsets),
                        recursive_dest_block->get_child_shape(recursive_child_pos, dest_shape));

                job.run();
            }
            return true;
        }

        /**
         * Visitor to handle case where result will be an inner block.
         */
        class recursive_visitor_t {
        public:
            explicit recursive_visitor_t(array<pair_set_t<LT, RT, CONFIG>, 4>& ret_sets) : ret_sets(ret_sets) {}

            /**
             * Reached two inner blocks. Recurse on children and modify offsets accordingly.
             *
             *        A      x      B      =       C
             *
             *   |----|----|   |----|----|   |-------------------------------|-------------------------------|
             *   | NW | NE |   | NW | NE |   | (A.NW * B.NW) + (A.NE * B.SW) | (A.NW * B.NE) + (A.NE * B.SE) |
             *   |----|----| x |----|----| = |-------------------------------|-------------------------------|
             *   | SW | SE |   | SW | SE |   | (A.SW * B.NW) + (A.SE * B.SW) | (A.SW * B.NE) + (A.SE * B.SE) |
             *   |----|----|   |----|----|   |-------------------------------|-------------------------------|
             *
             */
            void operator()(const std::shared_ptr<inner_block<LT, CONFIG>>& a, const std::shared_ptr<inner_block<RT, CONFIG>>& b) {
                // NW
                ret_sets[NW].pairs.emplace_back(a->get_child(NW), b->get_child(NW));
                ret_sets[NW].pairs.emplace_back(a->get_child(NE), b->get_child(SW));

                // NE
                ret_sets[NE].pairs.emplace_back(a->get_child(NW), b->get_child(NE));
                ret_sets[NE].pairs.emplace_back(a->get_child(NE), b->get_child(SE));

                // SW
                ret_sets[SW].pairs.emplace_back(a->get_child(SW), b->get_child(NW));
                ret_sets[SW].pairs.emplace_back(a->get_child(SE), b->get_child(SW));

                // SE
                ret_sets[SE].pairs.emplace_back(a->get_child(SW), b->get_child(NE));
                ret_sets[SE].pairs.emplace_back(a->get_child(SE), b->get_child(SE));
            }

            /**
             * Reached an inner block and a leaf block.
             *
             * @tparam LEAF_CAT
             */
            template <typename IT>
            void operator()(const std::shared_ptr<inner_block<LT, CONFIG>>& lhs, const leaf_category_t<RT, IT, CONFIG>& rhs) {
                throw not_implemented("shadow subdivision");
            }

            /**
             * Reached a leaf block and an inner block.
             *
             * @tparam LEAF_CAT
             */
            template <typename IT>
            void operator()(const leaf_category_t<LT, IT, CONFIG>& lhs, const std::shared_ptr<inner_block<RT, CONFIG>>& rhs) {
                throw not_implemented("shadow subdivision");
            }

            /**
             * Reached two leaf blocks.
             */
            template <typename IT>
            void operator()(const leaf_category_t<LT, IT, CONFIG>& lhs, const leaf_category_t<RT, IT, CONFIG>& rhs) {
                throw not_implemented("shadow subdivision");
            }

            /**
             * Reached a null block.
             */
            void operator()(const std::monostate& lhs, const std::monostate& rhs) {}

            template <typename OTHER>
            void operator()(const std::monostate& lhs, const OTHER& rhs) {}

            template <typename OTHER>
            void operator()(const OTHER& lhs, const std::monostate& rhs) {}

            /**
             * Reached a future block.
             */
            void operator()(const std::shared_ptr<future_block<LT, CONFIG>>& lhs, const std::shared_ptr<future_block<RT, CONFIG>>& rhs) {}

            /**
             * Reached a future block.
             * @tparam OTHER anything except std::monostate as that already has a handler above
             */
            template <typename OTHER, std::enable_if_t<std::is_same<OTHER, std::monostate>::value == 0>>
            void operator()(const std::shared_ptr<future_block<LT, CONFIG>>& lhs, const OTHER& rhs) {}

            /**
             * Reached a future block
             * @tparam OTHER anything except std::monostate as that already has a handler above
             */
            template <typename OTHER, std::enable_if_t<std::is_same<OTHER, std::monostate>::value == 0>>
            void operator()(const OTHER& lhs, const std::shared_ptr<future_block<RT, CONFIG>>& rhs) {}

            /**
             * Catch-all for combinations that should never happen. For example, mixing leaves of different categories.
             */
            template <typename LHS, typename RHS>
            void operator()(const LHS& lhs, const RHS& rhs) {
                // should not happen
                throw node_type_mismatch();
            }

        protected:
            array<pair_set_t<LT, RT, CONFIG>, 4>& ret_sets;
        };


        /**
         * pair_set contains only leaves. Multiply them.
         */
        bool multiply_leaves(unsigned pair_status) {
            leaf_index_type ret_type = get_leaf_index_type(dest_shape);

            if ((pair_status & HAS_LEAF_64) == HAS_LEAF_64) {
                return std::visit(result_leaf_visitor<int64_t>(*this), ret_type);
            } else if ((pair_status & HAS_LEAF_32) == HAS_LEAF_32) {
                return std::visit(result_leaf_visitor<int32_t>(*this), ret_type);
            } else if ((pair_status & HAS_LEAF_16) == HAS_LEAF_16) {
                return std::visit(result_leaf_visitor<int16_t>(*this), ret_type);
            } else {
                throw node_type_mismatch();
            }
        }

        /**
         * Visitor for the result block's index size. This index size is determined by the dimensions of the result, and
         * can potentially be different than the index size of the inputs.
         *
         * @tparam IT the input leaves' index size.
         */
        template <typename IT>
        class result_leaf_visitor {
        public:
            explicit result_leaf_visitor(const spawn_multiply_job<SR, CONFIG>& job): job(job) {}

            template <typename RETIT>
            bool operator()(RETIT retit) {
                dcsc_accumulator<RETT, RETIT, CONFIG> accumulator(job.dest_shape);
                leaf_category_visitor_t<IT, RETIT> visitor(job, accumulator);

                // multiply each pair in the pair list
                for (auto pair : job.pair_set.pairs) {
                    std::visit(visitor, pair.a, pair.b);
                }

                // collapse all the blocks
                auto result = accumulator.collapse(job.semiring);

                // write the new block to the result tree
                job.dest_bc->set_child(job.dest_position, result);

                return true;
            }

        protected:
            const spawn_multiply_job<SR, CONFIG>& job;
        };

        /**
         * Visitor for leaf categories. This simply unpacks the types and calls the concrete leaf visitor.
         */
        template <typename IT, typename RETIT>
        class leaf_category_visitor_t {
        public:
            explicit leaf_category_visitor_t(const spawn_multiply_job<SR, CONFIG>& job, dcsc_accumulator<RETT, RETIT, CONFIG> &accumulator) : job(job), accumulator(accumulator) {}

            /**
             * Visit categories.
             */
            void operator()(const leaf_category_t<LT, IT, CONFIG>& lhs, const leaf_category_t<RT, IT, CONFIG>& rhs) {
                std::visit(leaves_visitor_t<IT, RETIT>(job, accumulator), lhs, rhs);
            }

            /**
             * Visitor for everything else, which should never happen.
             */
            template <typename LHS, typename RHS>
            void operator()(const LHS& lhs, const RHS& rhs) {
                throw node_type_mismatch();
            }

        protected:
            dcsc_accumulator<RETT, RETIT, CONFIG>& accumulator;
            const spawn_multiply_job<SR, CONFIG>& job;
        };

        /**
         * Concrete leaf block visitor. The leaf block types are known here, so perform the multiplication.
         */
        template <typename IT, typename RETIT>
        class leaves_visitor_t {
        public:
            explicit leaves_visitor_t(const spawn_multiply_job<SR, CONFIG>& job, dcsc_accumulator<RETT, RETIT, CONFIG> &accumulator) : job(job), accumulator(accumulator) {}

            /**
             * Visit concrete leaf types and perform multiplication.
             */
            template <typename LHS, typename RHS>
            void operator()(const LHS& lhs, const RHS& rhs) {
                auto result = multiply_pair<IT, RETIT, SR, sparse_spa<RETIT, SR, CONFIG>, CONFIG>(lhs, rhs);
                accumulator.add(result);
            }

        protected:
            dcsc_accumulator<RETT, RETIT, CONFIG>& accumulator;
            const spawn_multiply_job<SR, CONFIG>& job;
        };

    protected:

        /**
         * Inputs
         */
        pair_set_t<LT, RT, CONFIG> pair_set;

        /**
         * Where to write result.
         */
        std::shared_ptr<block_container<RETT, CONFIG>> dest_bc;

        /**
         * position in dest_bc to write result to
         */
        int dest_position;

        /**
         * Offset within the destination matrix where the result will be. Used for task prioritization only.
         */
        offset_t dest_offsets;

        /**
         * Shape of the destination block. Needed to know the index size of the result block. Also used for SpA sizing.
         */
        shape_t dest_shape;

        /**
         * Semiring to use.
         */
        const SR& semiring;
    };
}

#endif //QUADMAT_MULTIPLY_TREES_H
