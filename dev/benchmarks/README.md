# Scalar array geometry benchmarks

`array_view_geometry_benchmark` compares three ways of constructing a
`GeoArrowGeometry` from each element of a native array:

- `GetGeometry`: `GeoArrowArrayViewGetGeometry()`, which borrows coordinates.
- `BorrowingVisitor`: `GeoArrowArrayViewVisitNative()` with a benchmark-local
  visitor that also borrows coordinates. This isolates traversal and node-building
  overhead from coordinate-copying costs.
- `Visitor`: `GeoArrowArrayViewVisitNative()` with `GeoArrowGeometryInitVisitor()`,
  which copies coordinates into geometry-owned storage.

The benchmark covers all six native geometry types, both coordinate layouts, and
XY/XYZ/XYM/XYZM dimensions. Sequences contain either 4 or 1024 coordinates (points
always contain one). Polygon/multilinestring containers have four children;
multipolygons have four polygons with four rings each. Each timed iteration reads
64 features individually. Output allocations are warmed and reused, and the first
and last geometries are checked for equal nodes and coordinate values before timing.
Fixtures are non-null and unsliced; the array-view unit tests cover nulls, empty
geometries, and slicing separately.

From the repository root:

```sh
cmake -S dev/benchmarks -B build/benchmarks --preset local
cmake --build build/benchmarks --target array_view_geometry_benchmark -j 4
taskset -c 2 build/benchmarks/array_view_geometry_benchmark \
  --benchmark_min_time=0.02s \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true \
  --benchmark_enable_random_interleaving=true \
  --benchmark_out=build/benchmarks/array_view_geometry.json
```

`taskset` is optional and Linux-specific; select an available CPU on your machine.
Reported Google Benchmark times are for a batch of 64 features; divide by 64 for
per-feature latency, or use the reported items-per-second counter directly.

## Example results

AMD Ryzen 9 5900X, GCC 15.2, CMake Release build, Google Benchmark 1.8.3, pinned
CPU core, five repetitions with randomly interleaved benchmark cases. CPU frequency
scaling remained enabled, so small differences should be treated as noise.
These are steady-state geometry construction times, not end-to-end processing times.
Borrowing paths avoid reading/copying coordinate contents during construction.

Across 88 combinations, the accessor was 1.77–3.89 times faster than the borrowing
visitor (median speedup 2.49), and 4.08–533.97 times faster than the copying visitor.
The large copying-visitor speedups come from avoiding coordinate copies, especially
for long linestrings and polygon rings.

Representative XY/separate medians, in nanoseconds per feature:

| Geometry | Coordinates per sequence | GetGeometry | Borrowing visitor | Copying visitor |
| --- | ---: | ---: | ---: | ---: |
| POINT | 1 | 6.1 | 12.2 | 25.6 |
| LINESTRING | 4 | 6.3 | 11.7 | 27.2 |
| LINESTRING | 1024 | 6.3 | 11.7 | 698.5 |
| POLYGON | 4 | 15.6 | 37.5 | 73.9 |
| POLYGON | 1024 | 15.5 | 37.5 | 2765.5 |
| MULTIPOINT | 4 | 14.7 | 35.6 | 61.8 |
| MULTIPOINT | 1024 | 2226.3 | 7109.5 | 11419.6 |
| MULTILINESTRING | 4 | 15.5 | 37.1 | 72.2 |
| MULTILINESTRING | 1024 | 15.6 | 37.1 | 2762.2 |
| MULTIPOLYGON | 4 | 55.1 | 144.8 | 264.1 |
| MULTIPOLYGON | 1024 | 55.2 | 144.8 | 11108.9 |
