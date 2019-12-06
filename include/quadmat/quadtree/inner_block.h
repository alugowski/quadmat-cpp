// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INNER_BLOCK_H
#define QUADMAT_INNER_BLOCK_H

#include <algorithm>
#include <array>

using std::array;
using std::shared_ptr;

#include "quadmat/quadtree/block_container.h"
#include "quadmat/quadtree/tree_nodes.h"

namespace quadmat {

    /**
     * Inner block child positions
     */
    enum inner_position {
        NW=0,
        NE=1,
        SW=2,
        SE=3
    };

    static constexpr std::initializer_list<inner_position> all_inner_positions = {NW, NE, SW, SE};

    /**
     * Block for inner nodes of the quad tree.
     *
     * @tparam T value type of leaf nodes
     */
    template<typename T, typename CONFIG>
    class inner_block : public block<T>, public block_container<T, CONFIG> {
    public:
        explicit inner_block(const index_t discriminating_bit) : discriminating_bit(discriminating_bit) {
            if (discriminating_bit == 0 || clear_all_except_msb(discriminating_bit) != discriminating_bit) {
                throw std::invalid_argument("invalid discriminating bit");
            }
        }

        [[nodiscard]] size_t num_children() const override {
            return all_inner_positions.size();
        }

        tree_node_t<T, CONFIG> get_child(int pos) const override {
            return children[pos];
        }

        void set_child(int pos, tree_node_t<T, CONFIG> child) override {
            children[pos] = child;
        }

        std::shared_ptr<inner_block<T, CONFIG>> create_inner(int pos) override {
            std::shared_ptr<inner_block<T, CONFIG>> ret =
                    std::make_shared<inner_block<T, CONFIG>>(this->discriminating_bit >> 1);
            children[pos] = ret;
            return ret;
        }

        [[nodiscard]] offset_t get_offsets(int child_pos, const offset_t& my_offset) const override {
            switch (child_pos) {
                case NW:
                    return my_offset;
                case NE:
                    return {
                            .row_offset = my_offset.row_offset,
                            .col_offset = my_offset.col_offset | discriminating_bit // NOLINT(hicpp-signed-bitwise)
                    };
                case SW:
                    return {
                            .row_offset = my_offset.row_offset | discriminating_bit, // NOLINT(hicpp-signed-bitwise),
                            .col_offset = my_offset.col_offset
                    };
                case SE:
                    return {
                            .row_offset = my_offset.row_offset | discriminating_bit, // NOLINT(hicpp-signed-bitwise),
                            .col_offset = my_offset.col_offset | discriminating_bit // NOLINT(hicpp-signed-bitwise)
                    };
                default:
                    throw std::invalid_argument("invalid child position");
            }
        }

        [[nodiscard]] shape_t get_child_shape(int child_pos, const shape_t& my_shape) const override {
            shape_t nw_shape = {
                    .nrows = std::min(discriminating_bit, my_shape.nrows),
                    .ncols = std::min(discriminating_bit, my_shape.ncols)
            };

            switch (child_pos) {
                case NW:
                    return nw_shape;
                case NE: // same number of rows as NW
                    return {
                            .nrows = nw_shape.nrows,
                            .ncols = my_shape.ncols - nw_shape.ncols
                    };
                case SW: // same number of columns as NW
                    return {
                            .nrows = my_shape.nrows - nw_shape.nrows,
                            .ncols = nw_shape.ncols
                    };
                case SE: // remainder of both rows and columns
                    return {
                            .nrows = my_shape.nrows - nw_shape.nrows,
                            .ncols = my_shape.ncols - nw_shape.ncols
                    };
                default:
                    throw std::invalid_argument("invalid child position");
            }
        }

        [[nodiscard]] index_t get_discriminating_bit() const override {
            return discriminating_bit;
        }

        [[nodiscard]] block_size_info size() const {
            return block_size_info{
                    .overhead_bytes = sizeof(inner_block<T, CONFIG>),
            };
        }

    protected:
        index_t discriminating_bit;
        array<tree_node_t<T, CONFIG>, 4> children{};
    };
}

#endif //QUADMAT_INNER_BLOCK_H
