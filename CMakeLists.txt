cmake_minimum_required(VERSION 3.14)
project(quadmat)

# DEPENDENCIES
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(TBB REQUIRED)
message(STATUS "Using Threading Building Blocks (TBB) version: " ${TBB_VERSION} )

# QuadMat project
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
add_library(quadmat include/quadmat.h include/matrix.h include/inner_block.h include/block.h include/config.h include/triples_block.h include/util.h include/generators.h include/generators/identity_tuples_generator.h include/generators/base_tuple_iterators.h include/generators/tuple_generators.h include/csc_block.h include/generators/simple_tuple_generator.h include/dcsc_block.h)
set_target_properties(quadmat PROPERTIES LINKER_LANGUAGE CXX)

#target_link_libraries(quadmat tbb)

# TESTS
add_subdirectory(test)