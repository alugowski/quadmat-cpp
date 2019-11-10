// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_QUADMAT_H
#define QUADMAT_QUADMAT_H

#include <iostream>

namespace quadmat {
}

// utilities
#include "util.h"

// quad tree
#include "tree_nodes.h"
#include "tree_visitors.h"
#include "dcsc_accumulator.h"
#include "single_block_container.h"
#include "matrix.h"

// generators
#include "generators.h"
#include "generators/tuple_generators.h"

// I/O
#include "io/simple_matrix_market.h"

// operations
#include "algorithms/multiply_trees.h"
#include "algorithms/multiply_leaves.h"

#endif //QUADMAT_QUADMAT_H
