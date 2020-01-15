project(benchmarks)

# Add Google Benchmark project and its Google Test dependency.
# Both are added as git submodules.
add_subdirectory(googletest)
add_subdirectory(benchmark)

# benchmark dense and sparse spa
add_executable(bench_spa spa_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_spa tbb benchmark)

# benchmark column lookup implementations
add_executable(bench_lookup lookup_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_lookup tbb benchmark)

# benchmark string parsing implementations
add_executable(bench_parse parse_bench.cpp benchmark_utilities.h)
target_link_libraries(bench_parse benchmark)