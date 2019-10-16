// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INNER_BLOCK_H
#define QUADMAT_INNER_BLOCK_H

#include <array>

using std::array;
using std::shared_ptr;

#include "block.h"

namespace quadmat {

    template<typename T>
    class block_container {
    public:
        int num_children() const;
    };

    template<typename T>
    class inner_block : public block<T> {
    protected:
        array<shared_ptr<block<T> >, 4> children;
    };
}

#endif //QUADMAT_INNER_BLOCK_H
