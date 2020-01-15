project(benchmarks)

# Add Google Benchmark project and its Google Test dependency.
# Both are added as git submodules.
add_subdirectory(googletest)
add_subdirectory(benchmark)

add_executable(bench_spa spa_bench.cpp)
target_link_libraries(bench_spa tbb benchmark)
