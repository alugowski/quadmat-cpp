// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_MATRIX_H
#define QUADMAT_MATRIX_H

#include <memory>

using std::shared_ptr;

#include "quadmat/quadtree/tree_nodes.h"
#include "quadmat/config.h"

namespace quadmat {

    template <typename T, typename CONFIG = default_config>
    class matrix {
    public:
        /**
         * Construct an empty matrix
         *
         * @param shape number of rows and columns.
         */
        explicit matrix(const shape_t& shape) : shape(shape), root_bc(std::make_shared<single_block_container<T, CONFIG>>(shape)) {}

        /**
         * Construct a matrix with a particular node
         * @param root_node
         * @param shape
         */
        matrix(const shape_t& shape, const tree_node_t<T, CONFIG>& root_node) : shape(shape), root_bc(std::make_shared<single_block_container<T, CONFIG>>(shape, root_node)) {}

        /**
         * @return the shape of this matrix
         */
        [[nodiscard]] shape_t get_shape() const { return shape; }

        /**
         * Get the number of non-null entries in this matrix.
         *
         * WARNING: this is O(# of blocks).
         *
         * @return number of non-nulls in this matrix
         */
        [[nodiscard]] size_t get_nnn() const {
            std::atomic<size_t> nnn{0};

            std::visit(leaf_visitor<T, CONFIG>([&](auto leaf, offset_t, shape_t) {
                nnn.fetch_add(leaf->nnn());
            }), get_root_bc()->get_child(0));

            return nnn;
        }

        /**
         * @return a const reference to the block container that holds the root tree node
         */
        std::shared_ptr<const block_container<T, CONFIG>> get_root_bc() const { return std::static_pointer_cast<const block_container<T, CONFIG>>(root_bc); }

        /**
         * Used for matrix modification.
         *
         * @return a non-const reference to the block container that holds the root tree node
         */
        std::shared_ptr<block_container<T, CONFIG>> get_root_bc() { return std::static_pointer_cast<block_container<T, CONFIG>>(root_bc); }

    protected:
        shape_t shape;

        std::shared_ptr<single_block_container<T, CONFIG>> root_bc;
    };

    /**
     * Construct a matrix from tuples.
     *
     * @tparam GEN tuple generator. Must have a begin() and end() that return tuple<IT, IT, T> for some integer type IT
     * @param shape shape of leaf
     * @param nnn estimated number of non-nulls, i.e. col_ordered_gen.size().
     * @param gen tuple generator
     */
    template <typename T, typename CONFIG = default_config, typename GEN>
    matrix<T, CONFIG> matrix_from_tuples(const shape_t shape, const blocknnn_t nnn, const GEN& gen) {
        matrix<T, CONFIG> ret{shape};

        // copy tuples into a triples block
        auto triples = std::make_shared<triples_block<T, index_t, CONFIG>>();
        triples->add(gen);

        // get a quadmat quadtree by subdividing the triples block
        auto subdivided_node = subdivide(triples, shape);

        ret.get_root_bc()->set_child(0, subdivided_node);
        return ret;
    }
}

#endif //QUADMAT_MATRIX_H
