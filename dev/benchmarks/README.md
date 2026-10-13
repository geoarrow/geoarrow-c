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
