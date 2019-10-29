// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INNER_BLOCK_H
#define QUADMAT_INNER_BLOCK_H

#include <algorithm>
#include <array>

using std::array;
using std::shared_ptr;

#include "block.h"

#include "tree_nodes.h"

namespace quadmat {

    /**
     * Block for inner nodes of the quad tree.
     *
     * @tparam T value type of leaf nodes
     */
    template<typename T, typename CONFIG>
    class inner_block : public block<T> {
    public:
        explicit inner_block(const shape_t& shape, const index_t discriminating_bit) : block<T>(shape), discriminating_bit(discriminating_bit) {}

        enum position {
            NW=0,
            NE=1,
            SW=2,
            SE=3
        };

        static constexpr std::initializer_list<position> all_positions = {NW, NE, SW, SE};

        tree_node_t<T, CONFIG> get_child(position pos) const {
            return children[pos];
        }

        void set_child(position pos, tree_node_t<T, CONFIG> child) {
            children[pos] = child;
        }

        offset_t get_offsets(position child_pos, const offset_t& my_offset) {
            switch (child_pos) {
                case NW:
                    return my_offset;
                case NE:
                    return {
                            .row_offset = my_offset.row_offset,
                            .col_offset = my_offset.col_offset << 1 | discriminating_bit // NOLINT(hicpp-signed-bitwise)
                    };
                case SW:
                    return {
                            .row_offset = my_offset.row_offset << 1 | discriminating_bit, // NOLINT(hicpp-signed-bitwise),
                            .col_offset = my_offset.col_offset
                    };
                case SE:
                    return {
                            .row_offset = my_offset.row_offset << 1 | discriminating_bit, // NOLINT(hicpp-signed-bitwise),
                            .col_offset = my_offset.col_offset << 1 | discriminating_bit // NOLINT(hicpp-signed-bitwise)
                    };
            }
        }

//        block_size_info size() override {
//            // sum sizes of children
//            return std::accumulate(children.begin(), children.end(),
//                                   block_size_info{
//                                           .overhead_bytes = sizeof(inner_block<T, CONFIG>),
//                                   },
//                                   [&](block_size_info acc, const tree_node_t<T, CONFIG>& child) {
//                                       return acc + child->size();
//                                   }
//            );
//        }

    protected:
        index_t discriminating_bit;
        array<tree_node_t<T, CONFIG>, 4> children{};
    };
}

#endif //QUADMAT_INNER_BLOCK_H
