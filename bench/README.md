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
BM_ScanSpeed                                16548076 ns     16498024 ns           41 3.17789G/s
BM_ChunkedScanSpeed                         40672226 ns     40634176 ns           17 1.29026G/s
BM_FindLineBreaks_strchr                    30845601 ns     30828955 ns           22 1.70064G/s
BM_FindLineBreaks_find_first_of_char        35178593 ns     35148750 ns           20 1.49163G/s
BM_FindLineBreaks_find_first_of_str         32520784 ns     32476333 ns           21 1.61437G/s
BM_FindLineBreaks_getline                  328721338 ns    328544000 ns            2 159.579M/s
BM_LineTokenize_strpbrk/space_only              82.5 ns         82.4 ns      7955449 509.696M/s
BM_LineTokenize_strpbrk/space_tab                110 ns          110 ns      6414192 380.868M/s
BM_LineTokenize_strtok                          92.9 ns         92.9 ns      7537824 452.242M/s
BM_IntFieldParse_from_chars                     21.9 ns         21.9 ns     31953986 319.748M/s
BM_IntFieldParse_atol                           41.3 ns         41.3 ns     17087716 169.557M/s
BM_IntFieldParse_stoll                          61.6 ns         61.5 ns     11828320 113.871M/s
BM_IntFieldParse_strtoll                        46.8 ns         46.6 ns     15113535 150.177M/s
BM_DoubleFieldParse_strtod                      82.9 ns         82.6 ns      9246539 169.559M/s
BM_LineParse_istringstream                      2011 ns         1996 ns       358831 21.0466M/s
BM_LineParse_sscanf                              755 ns          747 ns      1036055 56.2176M/s
BM_LineParse_strtoll                             247 ns          240 ns      3171425 175.324M/s
BM_LineParse_from_chars_strtod/space_only        189 ns          184 ns      3370960 227.985M/s
BM_LineParse_from_chars_strtod/space_tab         174 ns          174 ns      4019431 241.667M/s
BM_BlockParse_istringstream               2220819065 ns   2190635000 ns            1 23.9332M/s
BM_BlockParse_from_chars_strtod            253545326 ns    249363000 ns            3 210.251M/s
```