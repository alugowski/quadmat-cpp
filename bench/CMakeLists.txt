project(benchmarks)

# Add Google Benchmark project and its Google Test dependency.
# Both are added as git submodules.
add_subdirectory(googletest)
add_subdirectory(benchmark)

# benchmark configuration fields to find good default values
add_executable(bench_config_settings config_settings_bench.cpp ../test/test_utilities/problems.h)
target_compile_definitions(bench_config_settings PUBLIC TEST_MATRIX_DIR="${CMAKE_SOURCE_DIR}/test/matrices/")
target_link_libraries(bench_config_settings tbb benchmark)

# benchmark for validating and profiling in-progress changes
add_executable(bench_testing testing_bench.cpp ../test/test_utilities/problems.h)
target_compile_definitions(bench_testing PUBLIC TEST_MATRIX_DIR="${CMAKE_SOURCE_DIR}/test/matrices/")
target_link_libraries(bench_testing tbb benchmark)

# test if we should force std::filesystem fallback
# XCode 11 bundles a working implementation, but it its restricted to macOS 10.15 Catalina or later.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    message("Using fallback std::filesystem implementation because macOS.")
    target_compile_definitions(bench_config_settings PUBLIC QUADMAT_USE_GHC_FILESYSTEM=1)
    target_compile_definitions(bench_testing PUBLIC QUADMAT_USE_GHC_FILESYSTEM=1)
endif()

# benchmark dense and sparse spa
add_executable(bench_spa spa_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_spa tbb benchmark)

# benchmark column lookup implementations
add_executable(bench_lookup lookup_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_lookup tbb benchmark)

# benchmark string parsing implementations
add_executable(bench_parse parse_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_parse benchmark)

# binary search benchmarks
add_executable(bench_search search_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_search tbb benchmark)

# miscellaneous benchmarks
add_executable(bench_misc misc_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_misc benchmark)