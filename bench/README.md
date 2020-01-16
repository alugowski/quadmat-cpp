# Benchmarks

This directory includes micro and not-so-micro benchmarks to validate ideas and implementations. Not all benchmarked
ideas are implemented in the rest of the library, indeed many serve to show why something has been rejected.

The choice of what to benchmark is guided by profiling.

## Usage

Benchmarks are written using the [Google Benchmark](https://github.com/google/benchmark) library,
which itself uses the [Google Test](https://github.com/google/googletest) library.

Both are included here as git submodules.

If your `benchmark/` and `googletest/` are empty then use the following git command to fetch them:

```bash
git submodule update --init 
```

Benchmarks are small and grouped together so all related benchmarks are under one build target.
A single source file per target makes for easy development.

## Example

A good example of the value of micro benchmarks is parsing. Conventional wisdom says that disk I/O
is the bottleneck when loading data files, but indeed the conversion of characters to `int`s and `double`s
is often the bottleneck instead.

The simple approaches to parsing, such as standard C++ input streams, can have woeful performance.
Similarly some common but unsafe approaches are shown to not be fastest either.

The `bench_parse` benchmark shows that some simple separator finding and new C++ 17 `from_bytes` routines
can outperform simple input stream code by 10x, with `strtoll` and `strtod` close behind.

```
bench_parse --benchmark_counters_tabular=true
...
Run on (8 X 2800 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 6144 KiB (x1)
Load Average: 2.62, 2.79, 2.61
-----------------------------------------------------------------------------------------------
Benchmark                                          Time             CPU   Iterations      Bytes
-----------------------------------------------------------------------------------------------
BM_ScanSpeed                                16634453 ns     16575548 ns           42 3.16302G/s
BM_FindLineBreaks_strchr                    32272829 ns     32226773 ns           22 1.62687G/s
BM_FindLineBreaks_find_first_of_char        36833886 ns     36781895 ns           19  1.4254G/s
BM_FindLineBreaks_find_first_of_str         48736669 ns     48355625 ns           16 1084.23M/s
BM_FindLineBreaks_getline                  340889182 ns    340512000 ns            2 153.971M/s
BM_LineTokenize_strpbrk/space_only              84.9 ns         84.7 ns      8131215 495.763M/s
BM_LineTokenize_strpbrk/space_tab                113 ns          112 ns      6274707 373.646M/s
BM_IntFieldParse_from_chars                     22.8 ns         22.8 ns     31087761 307.048M/s
BM_IntFieldParse_atol                           42.7 ns         42.7 ns     16518706 164.021M/s
BM_IntFieldParse_stoll                          62.1 ns         61.9 ns     11306005 113.033M/s
BM_IntFieldParse_strtoll                        45.4 ns         45.3 ns     15507139  154.37M/s
BM_DoubleFieldParse_strtod                      79.7 ns         79.5 ns      8792755 176.033M/s
BM_LineParse_istringstream                      1961 ns         1953 ns       360735 21.5044M/s
BM_LineParse_sscanf                              712 ns          711 ns       934480 59.0656M/s
BM_LineParse_strtok_and_strtoll                  282 ns          281 ns      2452595 149.296M/s
BM_LineParse_strtoll                             218 ns          218 ns      3254406 193.102M/s
BM_LineParse_from_chars_strtod/space_only        170 ns          170 ns      4163742 247.344M/s
BM_LineParse_from_chars_strtod/space_tab         177 ns          177 ns      4007626 237.791M/s
BM_BlockParse_istringstream               2070970782 ns   2067699000 ns            1 25.3561M/s
BM_BlockParse_from_chars_strtod            230779056 ns    230515667 ns            3 227.441M/s
```