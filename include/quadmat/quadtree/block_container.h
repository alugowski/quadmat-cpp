// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_BLOCK_CONTAINER_H
#define QUADMAT_BLOCK_CONTAINER_H

#include "quadmat/quadtree/tree_nodes.h"

namespace quadmat {

    /**
     * A container of blocks.
     *
     * @tparam T
     */
    template<typename T, typename Config>
    class BlockContainer {
    public:
        [[nodiscard]] virtual size_t GetNumChildren() const = 0;

        virtual TreeNode<T, Config> GetChild(int pos) const = 0;

        virtual void SetChild(int pos, TreeNode<T, Config> child) = 0;

        virtual std::shared_ptr<InnerBlock<T, Config>> CreateInner(int pos) = 0;

        [[nodiscard]] virtual Offset GetChildOffsets(int child_pos, const Offset& my_offset) const = 0;

        [[nodiscard]] virtual Shape GetChildShape(int child_pos, const Shape& my_shape) const = 0;

        /**
         * @return an index_t with exactly 1 bit set. No child tuple may have this or any bits more significant set.
         */
        [[nodiscard]] virtual Index GetDiscriminatingBit() const = 0;
    };
}
#endif //QUADMAT_BLOCK_CONTAINER_H
