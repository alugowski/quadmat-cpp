// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INNER_BLOCK_H
#define QUADMAT_INNER_BLOCK_H

#include <algorithm>
#include <array>

#include "quadmat/quadtree/block_container.h"
#include "quadmat/quadtree/tree_nodes.h"
#include "quadmat/util/util.h"

namespace quadmat {

    /**
     * Inner block child positions
     */
    enum InnerPosition {
        NW=0,
        NE=1,
        SW=2,
        SE=3
    };

    static constexpr std::initializer_list<InnerPosition> kAllInnerPositions = {NW, NE, SW, SE};

    /**
     * Block for inner nodes of the quad tree.
     *
     * @tparam T value type of leaf nodes
     */
    template<typename T, typename Config>
    class InnerBlock : public Block<T>, public BlockContainer<T, Config> {
    public:
        explicit InnerBlock(const Index discriminating_bit) : discriminating_bit_(discriminating_bit) {
            if (discriminating_bit == 0 || ClearAllExceptMsb(discriminating_bit) != discriminating_bit) {
                throw std::invalid_argument("invalid discriminating bit");
            }
        }

        [[nodiscard]] size_t GetNumChildren() const override {
            return kAllInnerPositions.size();
        }

        TreeNode<T, Config> GetChild(int pos) const override {
            return children_[pos];
        }

        void SetChild(int pos, TreeNode<T, Config> child) override {
            children_[pos] = child;
        }

        std::shared_ptr<InnerBlock<T, Config>> CreateInner(int pos) override {
            std::shared_ptr<InnerBlock<T, Config>> ret =
                    quadmat::allocate_shared<Config, InnerBlock<T, Config>>(this->discriminating_bit_ >> 1);
            children_[pos] = ret;
            return ret;
        }

        [[nodiscard]] Offset GetChildOffsets(int child_pos, const Offset& my_offset) const override {
            switch (child_pos) {
                case NW:
                    return my_offset;
                case NE:
                    return {
                            .row_offset = my_offset.row_offset,
                            .col_offset = my_offset.col_offset | discriminating_bit_ // NOLINT(hicpp-signed-bitwise)
                    };
                case SW:
                    return {
                            .row_offset = my_offset.row_offset | discriminating_bit_, // NOLINT(hicpp-signed-bitwise),
                            .col_offset = my_offset.col_offset
                    };
                case SE:
                    return {
                            .row_offset = my_offset.row_offset | discriminating_bit_, // NOLINT(hicpp-signed-bitwise),
                            .col_offset = my_offset.col_offset | discriminating_bit_ // NOLINT(hicpp-signed-bitwise)
                    };
                default:
                    throw std::invalid_argument("invalid child position");
            }
        }

        [[nodiscard]] Shape GetChildShape(int child_pos, const Shape& my_shape) const override {
            Shape nw_shape = {
                    .nrows = std::min(discriminating_bit_, my_shape.nrows),
                    .ncols = std::min(discriminating_bit_, my_shape.ncols)
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

        [[nodiscard]] Index GetDiscriminatingBit() const override {
            return discriminating_bit_;
        }

        [[nodiscard]] BlockSizeInfo GetSize() const {
            return BlockSizeInfo{
                    .overhead_bytes = sizeof(InnerBlock<T, Config>),
            };
        }

    protected:
        Index discriminating_bit_;
        std::array<TreeNode<T, Config>, 4> children_{};
    };
}

#endif //QUADMAT_INNER_BLOCK_H
