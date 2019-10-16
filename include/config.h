// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_CONFIG_H
#define QUADMAT_CONFIG_H

#include <numeric>

namespace quadmat {
    typedef int32_t blocknnn_t;
    typedef int64_t index_t;

    class basic_settings {
    public:
        /**
         * leaf blocks larger than this should be split
         */
        static const blocknnn_t leaf_split_threshold = 10 * 1024;
    };
}

#endif //QUADMAT_CONFIG_H
