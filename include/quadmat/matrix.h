// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MATRIX_H
#define QUADMAT_MATRIX_H

#include <memory>

#include "quadmat/quadtree/tree_nodes.h"
#include "quadmat/config.h"

namespace quadmat {

    template <typename T, typename Config = DefaultConfig>
    class Matrix {
    public:
        /**
         * Construct an empty matrix
         *
         * @param shape number of rows and columns.
         */
        explicit Matrix(const Shape& shape) : shape_(shape), root_bc_(quadmat::allocate_shared<Config, single_block_container < T, Config>>(shape)) {}

        /**
         * Construct a matrix with a particular node
         * @param root_node
         * @param shape
         */
        Matrix(const Shape& shape, const TreeNode<T, Config>& root_node) : shape_(shape), root_bc_(quadmat::allocate_shared<Config, single_block_container < T, Config>>(shape, root_node)) {}

        /**
         * @return the shape of this matrix
         */
        [[nodiscard]] Shape GetShape() const { return shape_; }

        /**
         * Get the number of non-null entries in this matrix.
         *
         * WARNING: this is O(# of blocks).
         *
         * @return number of non-nulls in this matrix
         */
        [[nodiscard]] size_t GetNnn() const {
            std::atomic<size_t> nnn{0};

            std::visit(GetLeafVisitor<T, Config>([&](auto leaf, Offset, Shape) {
              nnn.fetch_add(leaf->GetNnn());
            }), GetRootBC()->GetChild(0));

            return nnn;
        }

        /**
         * @return a const reference to the block container that holds the root tree node
         */
        std::shared_ptr<const BlockContainer<T, Config>> GetRootBC() const { return std::static_pointer_cast<const BlockContainer<T, Config>>(root_bc_); }

        /**
         * Used for matrix modification.
         *
         * @return a non-const reference to the block container that holds the root tree node
         */
        std::shared_ptr<BlockContainer<T, Config>> GetRootBC() { return std::static_pointer_cast<BlockContainer<T, Config>>(root_bc_); }

    protected:
        Shape shape_;

        std::shared_ptr<single_block_container<T, Config>> root_bc_;
    };

    /**
     * Construct a matrix from tuples.
     *
     * @tparam Gen tuple generator. Must have a begin() and end() that return tuple<IT, IT, T> for some integer type IT
     * @param shape shape of leaf
     * @param nnn estimated number of non-nulls, i.e. col_ordered_gen.size().
     * @param gen tuple generator
     */
    template <typename T, typename Config = DefaultConfig, typename Gen>
    Matrix<T, Config> MatrixFromTuples(const Shape shape, const BlockNnn nnn, const Gen& gen) {
        Matrix<T, Config> ret{shape};

        // copy tuples into a triples block
        auto triples = quadmat::allocate_shared<Config, TriplesBlock<T, Index, Config>>();
        triples->Add(gen);

        // get a quadmat quadtree by subdividing the triples block
        auto subdivided_node = Subdivide(triples, shape);

        ret.GetRootBC()->SetChild(0, subdivided_node);
        return ret;
    }
}

#endif //QUADMAT_MATRIX_H
