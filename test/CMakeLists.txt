add_executable(unittest_run unit/quadmat/matrix_test.cpp unit/quadmat/test_tbb_present.cpp unit/quadmat/generators/tuple_generators_test.cpp unit/quadmat/util/util_test.cpp unit/quadmat/quadtree/leaf_blocks/triple_block_test.cpp unit/quadmat/quadtree/leaf_blocks/dcsc_block_test.cpp test_utilities/problem_generator.h unit/quadmat/algorithms/dcsc_accumulator_test.cpp unit/quadmat/algorithms/spa_test.cpp unit/quadmat/io/simple_matrix_market_test.cpp unit/quadmat/algorithms/multiply_test.cpp unit/quadmat/quadtree/tree_nodes_test.cpp test_utilities/testing_utilities.h unit/quadmat/quadtree/block_container_test.cpp unit/quadmat/quadtree/leaf_blocks/window_shadow_block_test.cpp unit/quadmat/quadtree/shadow_subdivision_test.cpp unit/quadmat/quadtree/tree_construction_test.cpp unit/quadmat/quadtree/inner_block_test.cpp unit/quadmat/quadtree/tree_visitors.cpp test_utilities/problem_loader.h test_utilities/tuple_dumper.h test_utilities/problem_structs.h test_utilities/problems.h)
target_link_libraries(unittest_run tbb)

add_executable(mediumtest_run medium/medium_main.cpp test_utilities/problem_loader.h test_utilities/tuple_dumper.h test_utilities/problem_structs.h test_utilities/problems.h)
target_link_libraries(mediumtest_run tbb)

# test if we should force std::filesystem fallback
# XCode 11 bundles a working implementation, but it its restricted to macOS 10.15 Catalina or later.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    message("Using fallback std::filesystem implementation because macOS.")
    target_compile_definitions(unittest_run PUBLIC QUADMAT_USE_GHC_FILESYSTEM=1)
    target_compile_definitions(mediumtest_run PUBLIC QUADMAT_USE_GHC_FILESYSTEM=1)
endif()

# enable code coverage
if ("${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    message("Building with llvm Code Coverage Tools")
    set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
elseif (CMAKE_COMPILER_IS_GNUCXX)
    message("Building with lcov Code Coverage Tools")
    set(CMAKE_CXX_FLAGS "--coverage")
endif()