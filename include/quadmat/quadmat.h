// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_QUADMAT_H
#define QUADMAT_QUADMAT_H

#include <iostream>

namespace quadmat {
}

// utilities
#include "quadmat/util/util.h"

// quad tree
#include "quadmat/quadtree/tree_nodes.h"
#include "quadmat/quadtree/tree_visitors.h"
#include "quadmat/algorithms/dcsc_accumulator.h"
#include "quadmat/quadtree/single_block_container.h"
#include "quadmat/matrix.h"

// generators
#include "quadmat/generators/matrix_generators.h"
#include "quadmat/generators/tuple_generators.h"

// I/O
#include "quadmat/io/simple_matrix_market.h"

// operations
#include "quadmat/algorithms/multiply_trees.h"
#include "quadmat/algorithms/multiply_leaves.h"

#endif //QUADMAT_QUADMAT_H
