cmake_minimum_required(VERSION 3.14)
project(quadmat)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
add_library(quadmat include/quadmat.h include/matrix.h include/inner_block.h include/block.h include/config.h include/triples_block.h include/util.h include/generators.h)
set_target_properties(quadmat PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory(test)