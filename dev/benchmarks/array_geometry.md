# Generic geometry I/O benchmarks

`GeoArrowArrayReaderRead()` reads one array element into a reusable
`GeoArrowGeometry`; `GeoArrowArrayWriterAppend()` accepts its geometry view.
`GeoArrowArrayWriterAppendNull()` explicitly appends a null. The C++ wrappers
expose `Read()`, `Append()`, and `AppendNull()`.

WKB reads copy nodes and borrow input coordinates. Native reads delegate to
`GeoArrowArrayViewGetGeometry()`. WKT reads use the geometry-building visitor,
which owns parsed coordinates. XY boxes also produce owned polygon coordinates.
Null reads produce a single type-zero node that `Append()` recognizes.

## Results

Measured on 2026-11-09, based on upstream `5671302`, with GCC 15.2.0,
CMake 4.2.3, Google Benchmark 1.8.3, and an AMD Ryzen 9 5900X.
Release build (`-O3 -DNDEBUG`), fast_float and Ryu enabled, pinned to CPU 2.
CPU frequency scaling/boost remained enabled. Values are ratios of median CPU
times from five randomly interleaved repetitions, with a 0.1-second minimum
per repetition.

**Speedup = visitor time / geometry time.** Values above 1 favor geometry;
values below 1 favor the visitor. Native means separated coordinates;
interleaved cases are listed separately. Full timings per feature and CPU-time
coefficients of variation are in [array_geometry_results.csv](array_geometry_results.csv).

| Conversion | Point | Line, 16 coords | Line, 1024 coords | Polygon, 128 coords | Multipolygon, 16 × 5 coords |
|---|---:|---:|---:|---:|---:|
| native → native | 0.27× | 0.41× | 0.85× | 0.66× | 0.68× |
| native → wkb | 0.38× | 1.00× | 2.54× | 1.97× | 1.70× |
| native → wkt | 0.53× | 1.00× | 1.04× | 1.04× | 1.01× |
| wkb → native | 0.56× | 1.86× | 7.44× | 4.57× | 1.85× |
| wkb → wkb | 0.69× | 2.50× | 8.43× | 5.61× | 2.52× |
| wkb → wkt | 0.75× | 1.14× | 1.17× | 1.17× | 1.31× |
| wkt → native | 0.42× | 0.84× | 0.95× | 0.94× | 0.75× |
| wkt → wkb | 0.48× | 0.94× | 1.03× | 1.02× | 0.95× |
| wkt → wkt | 0.55× | 0.96× | 1.02× | 1.02× | 0.96× |
| interleaved → wkb | 0.38× | 0.93× | 2.52× | 1.95× | 1.65× |
| wkb → interleaved | 0.56× | 1.87× | 7.65× | 4.65× | 1.89× |

The strongest consistent gains are WKB-to-WKB for long lines (8.43×) and
polygons (5.61×), and WKB-to-native for polygons (4.57×). Native-to-WKB improves
for long lines (2.54×) and multipart geometries (1.70×). Point conversions are
slower through geometry: native-to-native is 3.71× slower and native-to-WKB is
2.63× slower. Native-to-native favors the visitor for every shape tested.
Most WKT-input cases are slower, with the larger WKT-to-WKB/WKT cases close
to parity. Small differences near 1× should not drive API selection.

The long-line WKB-to-native visitor case was sensitive to run conditions:
four primary samples were 219–221 µs/batch and one was 503 µs (CV 45.7%).
A separate seven-repetition, 0.3-second run measured medians of 496 µs for the
visitor and 30.5 µs for geometry (16.26×; CVs 2.7% and 1.2%). The table retains
the original 7.44× median ratio rather than substituting the larger follow-up
result. The other primary cases had CVs below 7.3%.

These results are consistent with avoiding coordinate staging for larger WKB
sequences, while constructing and dispatching a geometry per feature costs more
for points. WKT still builds an intermediate geometry through callbacks. These
are synthetic shape comparisons, not a claim that one path is universally faster.

## Workload and timing

Each batch repeats a fixed XY geometry, with every sixteenth feature null:
4096 points, 1024 short lines, 32 long lines, 128 polygons, or 256 multipolygons.
Lines and polygon rings sample a circle; multipart polygons use translated rings.
The fixture is created before timing. All nine native/WKB/WKT conversion pairs,
plus WKB/interleaved in both directions, are measured for each shape (110 cases).

The visitor path calls `GeoArrowArrayReaderVisit()` once per batch with the
writer visitor. The geometry path loops over `Read()` and `Append()`. Both
reuse the same reader, writer, and geometry allocations across iterations.
Output allocation, `Finish()`, and output release are included; input creation,
initialization, and validation are excluded. Before each benchmark, both outputs
are decoded with the existing visitor into WKB and compared byte-for-byte,
including offsets and validity bits.

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

Omit `taskset` on systems without it, or select an available CPU. To reproduce
the follow-up, add `--benchmark_filter='^line1024/wkb_to_native/'` and use
`--benchmark_min_time=0.3s --benchmark_repetitions=7`.

## Correctness checks

887 tests passed in the Debug build and in the namespaced bundled build
(with fast_float/Ryu disabled for the bundle). The WKX fixture corpus was enabled.
Valgrind found no errors or leaks in the array reader/writer tests or the added
opposite-endian WKT regression test. Coverage includes nulls, slices, all native
geometry types/dimensions/layouts, large and view serialized arrays, empty parts,
reader/output lifetimes, writer reuse, and malformed serialized input.

The new WKB-to-WKT path exposed an existing direct WKT writer bug: coordinate
formatting and empty-point detection ignored the byte-swap flag. This change
also fixes those operations and tests regular and empty points, linestrings,
and flat/verbose multipoints with opposite-endian coordinates.
