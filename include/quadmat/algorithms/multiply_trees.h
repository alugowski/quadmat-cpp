// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MULTIPLY_TREES_H
#define QUADMAT_MULTIPLY_TREES_H

#include <type_traits>

#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/quadtree/shadow_subdivision.h"
#include "quadmat/algorithms/dcsc_accumulator.h"
#include "quadmat/algorithms/multiply_leaves.h"

namespace quadmat {

    /**
     * Compute the shape of the product of matrices a and b.
     *
     * @param a_shape shape of matrix a
     * @param b_shape shape of matrix b
     * @return shape of matrix a*b
     */
    inline Shape GetMultiplyResultShape(const Shape& a_shape, const Shape& b_shape) {
        return Shape{
                .nrows = a_shape.nrows,
                .ncols = b_shape.ncols
        };
    }

    enum PairStatus {
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
     * @tparam TypeA block A's type
     * @tparam TypeB block B's type
     * @tparam Config configuration shared by both blocks
     */
    template <typename TypeA, typename TypeB, typename Config>
    struct TreeNodePair {
        TreeNode<TypeA, Config> a;
        TreeNode<TypeB, Config> b;
        Shape a_shape;
        Shape b_shape;
        Index a_parent_disc_bit;
        Index b_parent_disc_bit;

        TreeNodePair(
                const TreeNode<TypeA, Config> &a, const TreeNode<TypeB, Config> &b,
                const Shape& a_shape, const Shape& b_shape,
                const Index a_parent_disc_bit, const Index b_parent_disc_bit
                ) : a(a), b(b), a_shape(a_shape), b_shape(b_shape),
                a_parent_disc_bit(a_parent_disc_bit), b_parent_disc_bit(b_parent_disc_bit) {}

        /**
         * Visitor that describes the contents of this TreeNodePair.
         */
        template <typename T>
        struct StatusVisitor {
            unsigned operator()(const std::monostate& ignored) {
                return HAS_EMPTY;
            }

            unsigned operator()(const std::shared_ptr<FutureBlock<T, Config>>& fb) {
                return HAS_FUTURE;
            }

            unsigned operator()(const std::shared_ptr<InnerBlock<T, Config>>& inner) {
                return HAS_INNER;
            }

            unsigned operator()(const LeafNode<T, Config> &leaf) {
                return HAS_LEAF;
            }
        };

        /**
         * Get a summary of what is inside this pair.
         *
         * @return a bitwise OR of status_t enum states
         */
        [[nodiscard]] unsigned GetStatus() const {
            unsigned a_status = std::visit(StatusVisitor<TypeA>{}, a);
            unsigned b_status = std::visit(StatusVisitor<TypeB>{}, b);

            if (a_shape.ncols != b_shape.nrows) {
                return HAS_MISMATCHED_DIMS;
            }

            return a_status | b_status;
        }

        LeafNode<TypeA, Config> GetLeafA() const {
            return std::get<LeafNode<TypeA, Config>>(a);
        }

        LeafNode<TypeA, Config> GetLeafB() const {
            return std::get<LeafNode<TypeA, Config>>(b);
        }
    };

    /**
     * A set of block pairs that when multiplied then summed will result in the output block.
     *
     * @tparam AT side a's type
     * @tparam BT side b's type
     * @tparam Config configuration shared by both sides
     */
    template <typename AT, typename BT, typename Config>
    struct PairSet {
        std::vector<TreeNodePair<AT, BT, Config>, typename Config::template TempAllocator<TreeNodePair<AT, BT, Config>>> pairs;

        PairSet() = default;
        explicit PairSet(
                const TreeNode<AT, Config>& a, const TreeNode<AT, Config>& b,
                const Shape& a_shape, const Shape& b_shape,
                const Index a_parent_disc_bit, const Index b_parent_disc_bit) : pairs{{a, b, a_shape, b_shape, a_parent_disc_bit, b_parent_disc_bit}} {}

        /**
         * Prune any pair in the pair set that has an empty block. The result of that multiplication is also an empty block.
         *
         * @return the bitwise OR of all statuses of remaining pairs, or 0 if empty.
         */
        unsigned PruneEmpty(bool prune_ok = true) {
            unsigned ret = 0;
            for (auto cur = pairs.begin(); cur != pairs.end();) {
                unsigned pair_status = cur->GetStatus();
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
        [[nodiscard]] std::pair<Index, Index> GetParentDiscriminatingBits() const {
            Index a = 0, b = 0;
            for (auto pair : pairs) {
                a |= pair.a_parent_disc_bit;
                b |= pair.b_parent_disc_bit;
            }
            return std::pair<Index, Index>{a, b};
        }
    };

    /**
     * Constructs and organizes the tasks necessary to multiply two quadmat trees.
     *
     * @tparam Semiring semiring to use
     * @tparam Config configuration
     */
    template <class Semiring, typename Config = DefaultConfig>
    class SpawnMultiplyJob {
    public:
        using AT = typename Semiring::MapTypeA;
        using BT = typename Semiring::MapTypeB;
        using RetT = typename Semiring::ReduceType;

        SpawnMultiplyJob(
                const PairSet<AT, BT, Config> &pair_set,
                std::shared_ptr<BlockContainer<RetT, Config>> dest_bc,
                int dest_position,
                const Offset &dest_offsets,
                const Shape& dest_shape,
                const Semiring& semiring = Semiring())
                : pair_set(pair_set), dest_bc(dest_bc), dest_position(dest_position), dest_offsets(dest_offsets), dest_shape(dest_shape), semiring(semiring) {
        }

        /**
         * Run multiply job.
         */
        bool Run(bool prune = true) {
            // see what needs to be done
            unsigned status = pair_set.PruneEmpty(prune);

            // see if pair set has anything to do
            if (status == EMPTY_PAIR) {
                // empty
                return true;
            }

            // sanity check
            if (dest_shape.ncols <= 0 || dest_shape.nrows <= 0) {
                throw NodeTypeMismatch("dest dimensions <= 0");
            }

            // sanity check
            if ((status & HAS_MISMATCHED_DIMS) == HAS_MISMATCHED_DIMS) {
                throw NodeTypeMismatch();
            }

            // check if there are any future blocks then we must wait for them
            if ((status & HAS_FUTURE) == HAS_FUTURE) {
                throw NotImplemented("waiting on future blocks");
            }

            // check if there are inner blocks
            if ((status & HAS_INNER) == HAS_INNER) {
                return Recurse();
            }

            // only leaves remaining, do multiplication
            return MultiplyLeaves(status);
        }

    protected:

        /**
         * pair_set contains an inner block. Put an InnerBlock at the destination, build pair sets for each one,
         * and recurse.
         */
        bool Recurse() {
            // build recursive pair sets
            std::array<PairSet<AT, BT, Config>, 4> recursive_pair_sets{};

            for (auto pair : pair_set.pairs) {
                std::visit(RecursiveVisitor(recursive_pair_sets, pair), pair.a, pair.b);
            }

            auto [a_parent_disc_bit, b_parent_disc_bit] = pair_set.GetParentDiscriminatingBits();
            Index a_disc_bit = a_parent_disc_bit >> 1;
            Index b_disc_bit = b_parent_disc_bit >> 1;

            if (a_disc_bit >= dest_bc->GetDiscriminatingBit()) {
                // the inputs are subdivided, but the result shouldn't be
                // For example, inputs might be a short-fat matrix * tall-skinny. Result has small dimensions and
                // does not require subdivision even though inputs do.

                // all 4 recursive pair sets should be merged into one
                PairSet<AT, BT, Config> recursive_pair_set;
                for (auto rec_pair_set_chunk : recursive_pair_sets) {
                    std::copy(rec_pair_set_chunk.pairs.begin(), rec_pair_set_chunk.pairs.end(), back_inserter(recursive_pair_set.pairs));
                }

                // spawn single recursive job
                SpawnMultiplyJob job(
                        recursive_pair_set,
                        dest_bc,
                        dest_position,
                        dest_offsets,
                        dest_shape);

                job.Run();
            } else {
                // construct result
                std::shared_ptr<InnerBlock<RetT, Config>> recursive_dest_block = dest_bc->CreateInner(dest_position);

                // recurse
                for (auto recursive_child_pos : kAllInnerPositions) {
                    SpawnMultiplyJob job(
                            recursive_pair_sets[recursive_child_pos],
                            recursive_dest_block,
                            recursive_child_pos,
                            recursive_dest_block->GetChildOffsets(recursive_child_pos, dest_offsets),
                            recursive_dest_block->GetChildShape(recursive_child_pos, dest_shape));

                    job.Run();
                }

                // clean up result
                CleanRecurseResult(recursive_dest_block);
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
         * @param recursive_dest_block InnerBlock that was produced.
         */
        void CleanRecurseResult(std::shared_ptr<InnerBlock<RetT, Config>>& recursive_dest_block) {
            bool empty = true;
            for (auto recursive_child_pos : kAllInnerPositions) {
                TreeNode<RetT, Config> child = recursive_dest_block->GetChild(recursive_child_pos);
                if (!std::holds_alternative<std::monostate>(child)) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                // all children are empty so remove the empty inner block
                dest_bc->SetChild(dest_position, std::monostate());
            }
        }

        /**
         * Visitor to handle case where result will be an inner block.
         */
        class RecursiveVisitor {
        public:
            explicit RecursiveVisitor(
                    std::array<PairSet<AT, BT, Config>, 4>& ret_sets,
                    const TreeNodePair<AT, BT, Config>& node_pair) : ret_sets_(ret_sets), node_pair_(node_pair) {}

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
            void operator()(const std::shared_ptr<InnerBlock<AT, Config>>& a, const std::shared_ptr<InnerBlock<BT, Config>>& b) {
                // NW
                ret_sets_[NW].pairs.emplace_back(a->GetChild(NW),
                                                 b->GetChild(NW),
                                                 a->GetChildShape(NW, node_pair_.a_shape),
                                                 b->GetChildShape(NW, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());
                ret_sets_[NW].pairs.emplace_back(a->GetChild(NE),
                                                 b->GetChild(SW),
                                                 a->GetChildShape(NE, node_pair_.a_shape),
                                                 b->GetChildShape(SW, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());

                // NE
                ret_sets_[NE].pairs.emplace_back(a->GetChild(NW),
                                                 b->GetChild(NE),
                                                 a->GetChildShape(NW, node_pair_.a_shape),
                                                 b->GetChildShape(NE, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());
                ret_sets_[NE].pairs.emplace_back(a->GetChild(NE),
                                                 b->GetChild(SE),
                                                 a->GetChildShape(NE, node_pair_.a_shape),
                                                 b->GetChildShape(SE, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());

                // SW
                ret_sets_[SW].pairs.emplace_back(a->GetChild(SW),
                                                 b->GetChild(NW),
                                                 a->GetChildShape(SW, node_pair_.a_shape),
                                                 b->GetChildShape(NW, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());
                ret_sets_[SW].pairs.emplace_back(a->GetChild(SE),
                                                 b->GetChild(SW),
                                                 a->GetChildShape(SE, node_pair_.a_shape),
                                                 b->GetChildShape(SW, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());

                // SE
                ret_sets_[SE].pairs.emplace_back(a->GetChild(SW),
                                                 b->GetChild(NE),
                                                 a->GetChildShape(SW, node_pair_.a_shape),
                                                 b->GetChildShape(NE, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());
                ret_sets_[SE].pairs.emplace_back(a->GetChild(SE),
                                                 b->GetChild(SE),
                                                 a->GetChildShape(SE, node_pair_.a_shape),
                                                 b->GetChildShape(SE, node_pair_.b_shape), a->GetDiscriminatingBit(),
                                                 b->GetDiscriminatingBit());
            }

            /**
             * Reached an inner block and a leaf block.
             */
            void operator()(const std::shared_ptr<InnerBlock<AT, Config>>& lhs, const LeafNode<BT, Config>& rhs) {
                auto rhs_inner = shadow_subdivide<BT, Config>(rhs, node_pair_.b_shape, node_pair_.b_parent_disc_bit);
                operator()(lhs, rhs_inner);
            }

            /**
             * Reached a leaf block and an inner block.
             */
            void operator()(const LeafNode<AT, Config>& lhs, const std::shared_ptr<InnerBlock<BT, Config>>& rhs) {
                auto lhs_inner = shadow_subdivide<AT, Config>(lhs, node_pair_.a_shape, node_pair_.a_parent_disc_bit);
                operator()(lhs_inner, rhs);
            }

            /**
             * Reached two leaf blocks. This happens when two leaves are in a pair set where another pair has an inner block.
             */
            void operator()(const LeafNode<AT, Config>& lhs, const LeafNode<BT, Config>& rhs) {
                auto lhs_inner = shadow_subdivide<AT, Config>(lhs, node_pair_.a_shape, node_pair_.a_parent_disc_bit);
                auto rhs_inner = shadow_subdivide<BT, Config>(rhs, node_pair_.b_shape, node_pair_.b_parent_disc_bit);
                operator()(lhs_inner, rhs_inner);
            }

            /**
             * Catch-all for combinations that should never happen. For example, mixing leaves of different categories.
             */
            template <typename LHS, typename RHS>
            void operator()(const LHS& lhs, const RHS& rhs) {
                // should not happen
                throw NodeTypeMismatch(std::string("catchall handler called with ( ") + typeid(lhs).name() + " , " + typeid(rhs).name() + " )");
            }

        protected:
            std::array<PairSet<AT, BT, Config>, 4>& ret_sets_;

            /**
             * The node pair that is being visited. Used for metadata such as shape.
             */
            const TreeNodePair<AT, BT, Config>& node_pair_;
        };


        /**
         * pair_set contains only leaves. Multiply them.
         */
        bool MultiplyLeaves(unsigned pair_status) {
            LeafIndex ret_type = GetLeafIndexType(dest_shape);
            return std::visit(ResultLeafVisitor(*this), ret_type);
        }

        /**
         * Visitor for the result block's index size. This index size is determined by the dimensions of the result, and
         * can potentially be different than the index size of the inputs.
         */
        class ResultLeafVisitor {
        public:
            explicit ResultLeafVisitor(const SpawnMultiplyJob<Semiring, Config>& job): job(job) {}

            template <typename RetIT>
            bool operator()(RetIT retit) {
                DcscAccumulator<RetT, RetIT, Config> accumulator(job.dest_shape);
                LeafCategoryPairMultiplyVisitor<RetIT> visitor(job, accumulator);

                // multiply each pair in the pair list
                for (auto pair : job.pair_set.pairs) {
                    std::visit(visitor, pair.GetLeafA(), pair.GetLeafB());
                }

                // collapse all the blocks
                auto result = accumulator.Collapse(job.semiring);

                // write the new block to the result tree
                if (result->GetNnn() > 0) {
                    job.dest_bc->SetChild(job.dest_position, LeafCategory<RetT, RetIT, Config>(result));
                } else {
                    job.dest_bc->SetChild(job.dest_position, std::monostate());
                }

                return true;
            }

        protected:
            const SpawnMultiplyJob<Semiring, Config>& job;
        };

        /**
         * Visitor for leaf categories. This simply unpacks the types and calls the concrete leaf visitor.
         */
        template <typename RetIT>
        class LeafCategoryPairMultiplyVisitor {
        public:
            explicit LeafCategoryPairMultiplyVisitor(const SpawnMultiplyJob<Semiring, Config>& job, DcscAccumulator<RetT, RetIT, Config> &accumulator) : job(job), accumulator(accumulator) {}

            /**
             * Unpack categories.
             */
            template <typename LHS, typename RHS>
            void operator()(const LHS& lhs, const RHS& rhs) {
                std::visit(LeafPairMultiplyVisitor<RetIT>(job, accumulator), lhs, rhs);
            }

        protected:
            DcscAccumulator<RetT, RetIT, Config>& accumulator;
            const SpawnMultiplyJob<Semiring, Config>& job;
        };

        /**
         * Concrete leaf block visitor. The leaf block types are known here, so perform the multiplication.
         */
        template <typename RetIT>
        class LeafPairMultiplyVisitor {
        public:
            explicit LeafPairMultiplyVisitor(const SpawnMultiplyJob<Semiring, Config>& job, DcscAccumulator<RetT, RetIT, Config> &accumulator) : job(job), accumulator(accumulator) {}

            /**
             * Visit concrete leaf types and perform multiplication.
             */
            template <typename LHS, typename RHS>
            void operator()(const std::shared_ptr<LHS>& lhs, const std::shared_ptr<RHS>& rhs) {
                if (Config::template ShouldUseDenseSpa<RetT>(job.dest_shape.nrows)) {
                    auto result = MultiplyPair<LHS, RHS, RetIT, Semiring, DenseSpa<RetIT, Semiring, Config>, Config>(lhs, rhs, job.dest_shape, job.semiring);
                    accumulator.Add(result);
                } else {
                    auto result = MultiplyPair<LHS, RHS, RetIT, Semiring, SparseSpa<RetIT, Semiring, Config>, Config>(lhs, rhs, job.dest_shape, job.semiring);
                    accumulator.Add(result);
                }
            }

        protected:
            DcscAccumulator<RetT, RetIT, Config>& accumulator;
            const SpawnMultiplyJob<Semiring, Config>& job;
        };

    protected:

        /**
         * Inputs
         */
        PairSet<AT, BT, Config> pair_set;

        /**
         * Where to write result.
         */
        std::shared_ptr<BlockContainer<RetT, Config>> dest_bc;

        /**
         * position in dest_bc to write result to
         */
        int dest_position;

        /**
         * Offset within the destination matrix where the result will be. Used for task prioritization only.
         */
        Offset dest_offsets;

        /**
         * Shape of the destination block. Needed to know the index size of the result block. Also used for SpA sizing.
         */
        Shape dest_shape;

        /**
         * Semiring to use.
         */
        const Semiring& semiring;
    };
}

#endif //QUADMAT_MULTIPLY_TREES_H
