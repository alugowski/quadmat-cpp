// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MULTIPLY_TREES_H
#define QUADMAT_MULTIPLY_TREES_H

#include <type_traits>
#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/quadtree/shadow_subdivision.h"
#include "quadmat/algorithms/multiply_leaves.h"

namespace quadmat {

    /**
     * Compute the shape of the product of matrices a and b.
     *
     * @param a_shape shape of matrix a
     * @param b_shape shape of matrix b
     * @return shape of matrix a*b
     */
    inline shape_t get_multiply_result_shape(const shape_t& a_shape, const shape_t& b_shape) {
        return shape_t{
                .nrows = a_shape.nrows,
                .ncols = b_shape.ncols
        };
    }

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

        /**
         * Has two leaves of different index sizes. Error condition that should never occur.
         */
        HAS_MISMATCHED_DIMS = 1u << 7u
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
        shape_t a_shape;
        shape_t b_shape;
        index_t a_parent_disc_bit;
        index_t b_parent_disc_bit;

        tree_node_pair_t(
                const tree_node_t<LT, CONFIG> &a, const tree_node_t<RT, CONFIG> &b,
                const shape_t& a_shape, const shape_t& b_shape,
                const index_t a_parent_disc_bit, const index_t b_parent_disc_bit
                ) : a(a), b(b), a_shape(a_shape), b_shape(b_shape),
                a_parent_disc_bit(a_parent_disc_bit), b_parent_disc_bit(b_parent_disc_bit) {}

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

            unsigned operator()(const leaf_node_t<T, CONFIG> &leaf) {
                return HAS_LEAF;
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

            if (a_shape.ncols != b_shape.nrows) {
                return HAS_MISMATCHED_DIMS;
            }

            return a_status | b_status;
        }

        leaf_node_t<LT, CONFIG> get_leaf_a() const {
            return std::get<leaf_node_t<LT, CONFIG>>(a);
        }

        leaf_node_t<LT, CONFIG> get_leaf_b() const {
            return std::get<leaf_node_t<LT, CONFIG>>(b);
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
        explicit pair_set_t(
                const tree_node_t<LT, CONFIG>& a, const tree_node_t<LT, CONFIG>& b,
                const shape_t& a_shape, const shape_t& b_shape,
                const index_t a_parent_disc_bit, const index_t b_parent_disc_bit) : pairs{{a, b, a_shape, b_shape, a_parent_disc_bit, b_parent_disc_bit}} {}

        /**
         * Prune any pair in the pair set that has an empty block. The result of that multiplication is also an empty block.
         *
         * @return the bitwise OR of all statuses of remaining pairs, or 0 if empty.
         */
        unsigned prune_empty(bool prune_ok = true) {
            unsigned ret = 0;
            for (auto cur = pairs.begin(); cur != pairs.end();) {
                unsigned pair_status = cur->get_status();
                if ((pair_status & HAS_EMPTY) == HAS_EMPTY && prune_ok) {
                    pairs.erase(cur);
                } else {
                    ret |= pair_status;
                    ++cur;
                }
            }

            return ret;
        }

        /**
         * @return a pair of discriminating bits, one for a and one for b sides
         */
        [[nodiscard]] std::pair<index_t, index_t> get_parent_discriminating_bits() const {
            index_t a = 0, b = 0;
            for (auto pair : pairs) {
                a |= pair.a_parent_disc_bit;
                b |= pair.b_parent_disc_bit;
            }
            return std::pair<index_t, index_t>{a, b};
        }
    };

    /**
     * Constructs and organizes the tasks necessary to multiply two quadmat trees.
     *
     * @tparam SR semiring to use
     * @tparam CONFIG configuration
     */
    template <class SR, typename CONFIG = default_config>
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
        bool run(bool prune = true) {
            // see what needs to be done
            unsigned status = pair_set.prune_empty(prune);

            // see if pair set has anything to do
            if (status == EMPTY_PAIR) {
                // empty
                return true;
            }

            // sanity check
            if ((status & HAS_MISMATCHED_DIMS) == HAS_MISMATCHED_DIMS) {
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
            // build recursive pair sets
            array<pair_set_t<LT, RT, CONFIG>, 4> recursive_pair_sets{};

            for (auto pair : pair_set.pairs) {
                std::visit(recursive_visitor_t(recursive_pair_sets, pair), pair.a, pair.b);
            }

            auto [a_parent_disc_bit, b_parent_disc_bit] = pair_set.get_parent_discriminating_bits();
            index_t a_disc_bit = a_parent_disc_bit >> 1;
            index_t b_disc_bit = b_parent_disc_bit >> 1;

            if (a_disc_bit >= dest_bc->get_discriminating_bit()) {
                // the inputs are subdivided, but the result shouldn't be
                // For example, inputs might be a short-fat matrix * tall-skinny. Result has small dimensions and
                // does not require subdivision even though inputs do.

                // all 4 recursive pair sets should be merged into one
                pair_set_t<LT, RT, CONFIG> recursive_pair_set;
                for (auto rec_pair_set_chunk : recursive_pair_sets) {
                    std::copy(rec_pair_set_chunk.pairs.begin(), rec_pair_set_chunk.pairs.end(), back_inserter(recursive_pair_set.pairs));
                }

                // spawn single recursive job
                spawn_multiply_job job(
                        recursive_pair_set,
                        dest_bc,
                        dest_position,
                        dest_offsets,
                        dest_shape);

                job.run();
            } else {
                // construct result
                std::shared_ptr<inner_block<RETT, CONFIG>> recursive_dest_block = dest_bc->create_inner(dest_position);

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

                // clean up result
                clean_recurse_result(recursive_dest_block);
            }

            return true;
        }

        /**
         * Do basic cleanup of a recursive multiply call. This only makes the structure cleaner, it does not change
         * the meaning of the matrix.
         *
         * Cleanup tasks include
         * - If all children of `recursive_dest_block` are empty then make the result empty too
         *
         * @param recursive_dest_block inner_block that was produced.
         */
        void clean_recurse_result(std::shared_ptr<inner_block<RETT, CONFIG>>& recursive_dest_block) {
            bool empty = true;
            for (auto recursive_child_pos : all_inner_positions) {
                tree_node_t<RETT, CONFIG> child = recursive_dest_block->get_child(recursive_child_pos);
                if (!std::holds_alternative<std::monostate>(child)) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                // all children are empty so remove the empty inner block
                dest_bc->set_child(dest_position, std::monostate());
            }
        }

        /**
         * Visitor to handle case where result will be an inner block.
         */
        class recursive_visitor_t {
        public:
            explicit recursive_visitor_t(
                    array<pair_set_t<LT, RT, CONFIG>, 4>& ret_sets,
                    const tree_node_pair_t<LT, RT, CONFIG>& node_pair) : ret_sets(ret_sets), node_pair(node_pair) {}

            /**
             * Reached two inner blocks.
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
                ret_sets[NW].pairs.emplace_back(a->get_child(NW), b->get_child(NW), a->get_child_shape(NW, node_pair.a_shape), b->get_child_shape(NW, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());
                ret_sets[NW].pairs.emplace_back(a->get_child(NE), b->get_child(SW), a->get_child_shape(NE, node_pair.a_shape), b->get_child_shape(SW, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());

                // NE
                ret_sets[NE].pairs.emplace_back(a->get_child(NW), b->get_child(NE), a->get_child_shape(NW, node_pair.a_shape), b->get_child_shape(NE, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());
                ret_sets[NE].pairs.emplace_back(a->get_child(NE), b->get_child(SE), a->get_child_shape(NE, node_pair.a_shape), b->get_child_shape(SE, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());

                // SW
                ret_sets[SW].pairs.emplace_back(a->get_child(SW), b->get_child(NW), a->get_child_shape(SW, node_pair.a_shape), b->get_child_shape(NW, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());
                ret_sets[SW].pairs.emplace_back(a->get_child(SE), b->get_child(SW), a->get_child_shape(SE, node_pair.a_shape), b->get_child_shape(SW, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());

                // SE
                ret_sets[SE].pairs.emplace_back(a->get_child(SW), b->get_child(NE), a->get_child_shape(SW, node_pair.a_shape), b->get_child_shape(NE, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());
                ret_sets[SE].pairs.emplace_back(a->get_child(SE), b->get_child(SE), a->get_child_shape(SE, node_pair.a_shape), b->get_child_shape(SE, node_pair.b_shape), a->get_discriminating_bit(), b->get_discriminating_bit());
            }

            /**
             * Reached an inner block and a leaf block.
             */
            void operator()(const std::shared_ptr<inner_block<LT, CONFIG>>& lhs, const leaf_node_t<RT, CONFIG>& rhs) {
                auto rhs_inner = shadow_subdivide<RT, CONFIG>(rhs, node_pair.b_shape, node_pair.b_parent_disc_bit);
                operator()(lhs, rhs_inner);
            }

            /**
             * Reached a leaf block and an inner block.
             */
            void operator()(const leaf_node_t<LT, CONFIG>& lhs, const std::shared_ptr<inner_block<RT, CONFIG>>& rhs) {
                auto lhs_inner = shadow_subdivide<LT, CONFIG>(lhs, node_pair.a_shape, node_pair.a_parent_disc_bit);
                operator()(lhs_inner, rhs);
            }

            /**
             * Reached two leaf blocks. This happens when two leaves are in a pair set where another pair has an inner block.
             */
            void operator()(const leaf_node_t<LT, CONFIG>& lhs, const leaf_node_t<RT, CONFIG>& rhs) {
                auto lhs_inner = shadow_subdivide<LT, CONFIG>(lhs, node_pair.a_shape, node_pair.a_parent_disc_bit);
                auto rhs_inner = shadow_subdivide<RT, CONFIG>(rhs, node_pair.b_shape, node_pair.b_parent_disc_bit);
                operator()(lhs_inner, rhs_inner);
            }

            /**
             * Catch-all for combinations that should never happen. For example, mixing leaves of different categories.
             */
            template <typename LHS, typename RHS>
            void operator()(const LHS& lhs, const RHS& rhs) {
                // should not happen
                throw node_type_mismatch(std::string("catchall handler called with ( ") + typeid(lhs).name() + " , " + typeid(rhs).name() + " )");
            }

        protected:
            array<pair_set_t<LT, RT, CONFIG>, 4>& ret_sets;

            /**
             * The node pair that is being visited. Used for metadata such as shape.
             */
            const tree_node_pair_t<LT, RT, CONFIG>& node_pair;
        };


        /**
         * pair_set contains only leaves. Multiply them.
         */
        bool multiply_leaves(unsigned pair_status) {
            leaf_index_type ret_type = get_leaf_index_type(dest_shape);
            return std::visit(result_leaf_visitor(*this), ret_type);
        }

        /**
         * Visitor for the result block's index size. This index size is determined by the dimensions of the result, and
         * can potentially be different than the index size of the inputs.
         */
        class result_leaf_visitor {
        public:
            explicit result_leaf_visitor(const spawn_multiply_job<SR, CONFIG>& job): job(job) {}

            template <typename RETIT>
            bool operator()(RETIT retit) {
                dcsc_accumulator<RETT, RETIT, CONFIG> accumulator(job.dest_shape);
                leaf_category_pair_multiply_visitor_t<RETIT> visitor(job, accumulator);

                // multiply each pair in the pair list
                for (auto pair : job.pair_set.pairs) {
                    std::visit(visitor, pair.get_leaf_a(), pair.get_leaf_b());
                }

                // collapse all the blocks
                auto result = accumulator.collapse(job.semiring);

                // write the new block to the result tree
                if (result->nnn() > 0) {
                    job.dest_bc->set_child(job.dest_position, leaf_category_t<RETT, RETIT, CONFIG>(result));
                } else {
                    job.dest_bc->set_child(job.dest_position, std::monostate());
                }

                return true;
            }

        protected:
            const spawn_multiply_job<SR, CONFIG>& job;
        };

        /**
         * Visitor for leaf categories. This simply unpacks the types and calls the concrete leaf visitor.
         */
        template <typename RETIT>
        class leaf_category_pair_multiply_visitor_t {
        public:
            explicit leaf_category_pair_multiply_visitor_t(const spawn_multiply_job<SR, CONFIG>& job, dcsc_accumulator<RETT, RETIT, CONFIG> &accumulator) : job(job), accumulator(accumulator) {}

            /**
             * Unpack categories.
             */
            template <typename LHS, typename RHS>
            void operator()(const LHS& lhs, const RHS& rhs) {
                std::visit(leaf_pair_multiply_visitor_t<RETIT>(job, accumulator), lhs, rhs);
            }

        protected:
            dcsc_accumulator<RETT, RETIT, CONFIG>& accumulator;
            const spawn_multiply_job<SR, CONFIG>& job;
        };

        /**
         * Concrete leaf block visitor. The leaf block types are known here, so perform the multiplication.
         */
        template <typename RETIT>
        class leaf_pair_multiply_visitor_t {
        public:
            explicit leaf_pair_multiply_visitor_t(const spawn_multiply_job<SR, CONFIG>& job, dcsc_accumulator<RETT, RETIT, CONFIG> &accumulator) : job(job), accumulator(accumulator) {}

            /**
             * Visit concrete leaf types and perform multiplication.
             */
            template <typename LHS, typename RHS>
            void operator()(const std::shared_ptr<LHS>& lhs, const std::shared_ptr<RHS>& rhs) {
                if (CONFIG::template use_dense_spa<RETT>(job.dest_shape.nrows)) {
                    auto result = multiply_pair<LHS, RHS, RETIT, SR, dense_spa<RETIT, SR, CONFIG>, CONFIG>(lhs, rhs, job.dest_shape, job.semiring);
                    accumulator.add(result);
                } else {
                    auto result = multiply_pair<LHS, RHS, RETIT, SR, sparse_spa<RETIT, SR, CONFIG>, CONFIG>(lhs, rhs, job.dest_shape, job.semiring);
                    accumulator.add(result);
                }
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
