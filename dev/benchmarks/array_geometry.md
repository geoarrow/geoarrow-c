# Generic geometry I/O benchmarks

`GeoArrowArrayReaderVisitGeometry()` processes a range with a function pointer
accepting a `GeoArrowGeometryView`, caller context, and `GeoArrowError*`, returning
an error code. `GeoArrowArrayWriterAppend()` consumes each view. The scalar
`GeoArrowArrayReaderRead()` remains available for random access or caller-owned
nodes. C++ wrappers expose `VisitGeometry()`, `Read()`, `Append()`, and `AppendNull()`.

## Batch API

For an initialized reader and writer:

```c
static GeoArrowErrorCode AppendGeometry(struct GeoArrowGeometryView geom,
                                        void* context, struct GeoArrowError* error) {
  return GeoArrowArrayWriterAppend((struct GeoArrowArrayWriter*)context, geom, error);
}

// After SetArray(), process the entire batch inside the reader:
GeoArrowArrayReaderVisitGeometry(&reader, 0, array.length,
                                 AppendGeometry, &writer, &error);
```

The callback is invoked once per feature, in order, including nulls represented
by a single type-zero node. Any nonzero return stops iteration immediately and
is returned unchanged. Views are only guaranteed valid during the callback;
use `GeoArrowGeometryDeepCopy()` to retain an owned value. The callback must not
use or modify the active reader. Reader-owned scratch storage is reused across
calls and released by `Reset()`.

The batch API validates the range and selects the input path once. WKB forwards
the parser's nodes directly, avoiding the scalar API's shallow copy. Native points
and linestrings reuse one node whose type, dimensions, and strides are initialized
outside the loop. Other native types use `GeoArrowArrayViewGetGeometry()`. WKT
still uses the geometry-building visitor and owns its parsed coordinates.

## Results

Measured on 2026-11-09 with GCC 15.2.0, CMake 4.2.3, Google Benchmark 1.8.3,
and an AMD Ryzen 9 5900X. Release build (`-O3 -DNDEBUG`), fast_float and Ryu
enabled, pinned to CPU 2. CPU scaling/boost remained enabled. The three strategies
were measured together using five randomly interleaved repetitions with a
0.1-second minimum. Every benchmark verified equivalent output before timing.

These measurements call the C writer API directly for all three strategies.
The earlier scalar benchmark used the throwing C++ `Append()` wrapper, which
zero-initializes a 1024-byte error buffer per feature. That overhead dominated
some point cases and obscured the effect of batching. This report and CSV replace
that comparison; they measure the underlying C APIs without that wrapper cost.

Times are median CPU nanoseconds per feature, including output allocation,
`Finish()`, and release. Lower is better. Speedup is baseline time divided by
callback time; values below 1 mean that the callback is slower.

| Shape / conversion | Event visitor | Scalar geometry | Callback geometry | Callback vs scalar | Callback vs visitor |
|---|---:|---:|---:|---:|---:|
| point: native → native | 13.2 | 28.9 | 21.2 | 1.36× | 0.62× |
| point: native → wkb | 14.8 | 20.8 | 12.7 | 1.64× | 1.17× |
| point: native → wkt | 28.6 | 34.5 | 26.8 | 1.29× | 1.06× |
| point: wkb → native | 29.9 | 39.2 | 27.7 | 1.42× | 1.08× |
| point: wkb → wkb | 34.0 | 25.4 | 19.5 | 1.30× | 1.75× |
| point: wkt → native | 37.1 | 70.7 | 64.0 | 1.11× | 0.58× |
| line16: native → native | 25.6 | 43.9 | 35.6 | 1.23× | 0.72× |
| line16: native → wkb | 51.3 | 36.1 | 29.5 | 1.22× | 1.74× |
| line16: wkb → wkb | 154.4 | 41.0 | 36.0 | 1.14× | 4.29× |
| line1024: wkb → wkb | 8482.9 | 1093.0 | 1083.3 | 1.01× | 7.83× |
| polygon128: wkb → native | 831.9 | 179.2 | 170.6 | 1.05× | 4.88× |
| multipolygon16x5: wkb → native | 866.0 | 487.0 | 437.7 | 1.11× | 1.98× |

The complete 55-conversion table and CPU-time coefficients of variation are in
[array_geometry_results.csv](array_geometry_results.csv). Native means separated
coordinates; the CSV also includes interleaved input/output comparisons. Small
differences near 1× should not drive API selection, especially when they are
comparable to the reported variation.

Batching reduces range checks, node copying, and repeated metadata setup, but
still calls the generic geometry writer for each feature. It does not make all
point conversions faster than the specialized event-visitor path. WKT still
pays to build an intermediate geometry. The interface also creates room for
further batch-level specialization without changing caller loops.

For reader/writer component timings and instruction attribution of the remaining
native-to-native overhead, see [the per-feature profile](array_geometry_profile.md).

## Workload and timing

Each batch repeats a fixed XY geometry, with every sixteenth feature null:
4096 points, 1024 lines of 16 coordinates, 32 lines of 1024 coordinates,
128 polygons of 128 coordinates, or 256 multipolygons with 16 five-coordinate
rings. Lines and polygon rings sample a circle; multipart polygons use translated
rings. These are synthetic shape comparisons, not a universal workload model.

All nine native/WKB/WKT conversions, plus WKB/interleaved in both directions,
are measured for each shape and strategy (165 cases). The event visitor calls
`GeoArrowArrayReaderVisit()` once per batch; scalar geometry loops over `Read()`
and `Append()`; callback geometry calls `VisitGeometry()` with a C `Append()`
adapter. Reader/writer initialization and input creation are excluded. Readers
and writers are reused, and the geometry paths retain scratch capacity across
iterations. Output allocation, finishing, and release are included.

Before timing, outputs from all three strategies are decoded with the existing
event visitor into WKB and compared byte-for-byte, including offsets and validity.

## Reproduce

From the repository root:

```sh
cmake -S dev/benchmarks -B build/bench -DCMAKE_BUILD_TYPE=Release \
  -DGEOARROW_BENCHMARK_SOURCE_URL="$PWD"
cmake --build build/bench --target array_geometry_benchmark -j

taskset -c 2 build/bench/array_geometry_benchmark \
  --benchmark_min_time=0.1s --benchmark_repetitions=5 \
  --benchmark_enable_random_interleaving=true \
  --benchmark_display_aggregates_only=true \
  --benchmark_out=array_geometry.json
```

Omit `taskset` on systems without it, or select an available CPU. Add
`--benchmark_filter='^point/'` to isolate the small-geometry cases.

## Correctness

891 tests passed in the Debug build and the namespaced bundled build (with
fast_float/Ryu disabled for the bundle); the WKX fixture corpus was enabled.
Valgrind reported no errors or leaks in the array reader tests, including callback
scratch storage and retained geometry lifetimes. Earlier checks of the writer
and opposite-endian WKT regression tests were also clean.

Coverage includes all native types/dimensions/layouts, serialized large/view
arrays, nulls, empty parts, independent array and coordinate-child offsets,
malformed input, early callback termination, error-pointer propagation, invalid
and overflowing ranges, reader reuse after parse errors, and deep-copied views.

The generic geometry path exposed two byte-order bugs addressed in this PR:
direct WKT formatting/empty-point detection ignored the swap flag, and deep
copies retained that flag after normalizing their coordinates to native byte
order. Regression tests cover both cases.
