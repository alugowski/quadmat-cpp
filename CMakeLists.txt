cmake_minimum_required(VERSION 3.14)
project(quadmat)

# DEPENDENCIES
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(TBB REQUIRED)
message(STATUS "Using Threading Building Blocks (TBB) version: " ${TBB_VERSION} )

# QuadMat project
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
add_library(quadmat include/quadmat/quadmat.h include/quadmat/matrix.h include/quadmat/quadtree/inner_block.h include/quadmat/quadtree/block.h include/quadmat/config.h include/quadmat/quadtree/leaf_blocks/triples_block.h include/quadmat/util/util.h include/quadmat/util/stream_chunker.h include/quadmat/generators/matrix_generators.h include/quadmat/generators/identity_tuples_generator.h include/quadmat/util/base_iterators.h include/quadmat/generators/tuple_generators.h include/quadmat/generators/simple_tuple_generator.h include/quadmat/quadtree/leaf_blocks/dcsc_block.h include/quadmat/util/types.h include/quadmat/algorithms/dcsc_accumulator.h include/quadmat/quadtree/block_container.h include/quadmat/algorithms/map_spa.h include/quadmat/io/simple_matrix_market.h include/quadmat/generators/full_tuples_generator.h include/quadmat/algorithms/multiply_leaves.h include/quadmat/quadtree/tree_nodes.h include/quadmat/algorithms/multiply_trees.h include/quadmat/quadtree/future_block.h include/quadmat/quadtree/single_block_container.h include/quadmat/algorithms/algorithms.h include/quadmat/quadtree/leaf_blocks/window_shadow_block.h include/quadmat/quadtree/shadow_subdivision.h include/quadmat/quadtree/tree_construction.h include/quadmat/algorithms/spa.h include/quadmat/algorithms/dense_spa.h include/quadmat/executors/direct_task_queue.h include/quadmat/executors/task.h)
set_target_properties(quadmat PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(quadmat tbb)

# TESTS
add_subdirectory(test)

# BENCHMARKS
add_subdirectory(bench)