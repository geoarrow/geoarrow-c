# Native-to-native per-feature profile

The remaining small-geometry gap is primarily in the native geometry writer.
The callback reader is cheaper than the event reader in isolation, and bypassing
`GeoArrowArrayWriterAppend()` saves only about 1 ns/feature. Native `Append()` still
counts coordinates, plans conversions and offsets, maintains validity, checks
capacity, and sets up generic coordinate copying for each feature.

These measurements extend [the conversion benchmark](array_geometry.md), using
the same separated XY inputs and one null per 16 features. No library code was
changed for this investigation.

## Component timings

Median CPU nanoseconds per feature, lower is better:

| Measurement | Point | Line, 16 coordinates |
|---|---:|---:|
| Full event-visitor conversion | 13.15 | 26.29 |
| Full scalar geometry conversion | 28.75 | 44.58 |
| Full geometry callback conversion | 21.00 | 36.83 |
| Read only, void event visitor | 6.08 | 5.63 |
| Read only, scalar geometry | 9.39 | 9.75 |
| Read only, geometry callback | 2.12 | 2.75 |
| Write cached views, generic writer | 20.23 | 34.26 |
| Write cached views, native writer | 19.61 | 34.20 |
| Full callback directly to native writer | 20.20 | 35.81 |

Read-only geometry callbacks consume the view with `DoNotOptimize()`; the event
reader uses `GeoArrowVisitorInitVoid()`. Cached-write benchmarks prepare all node
lists outside timing and still borrow coordinates from the input. Direct-native
callbacks retain null handling but bypass generic writer validation and dispatch.
Every write benchmark includes output allocation, `Finish()`, and release, and
checks equivalent output before timing.

These are separate experiments, not additive cost estimates. In particular,
caching every view changes the working set compared with reusing one scratch
view. Subtracting read-only time from full conversion time is not an exact writer
time measurement. Small differences should be interpreted alongside variation.

The multipart cases point in the same direction. Polygon128 and multipolygon16x5
callback reads take 10.0 and 75.3 ns/feature; full conversions take 168.7 and 343.2
ns. Event conversions take 122.4 and 270.7 ns. Bypassing generic writer dispatch
does not consistently improve these larger cases. All 36 timings and their
variation are in [the timing CSV](array_geometry_profile_results.csv).

## Instruction attribution

Hardware counters are unavailable on this host (`perf_event_paranoid=4`).
Callgrind 3.26.0 instead counts executed instructions in the optimized binary.
These percentages are **instruction shares, not CPU-time shares**; they do not
measure hardware branch mispredictions, cache stalls, or indirect-call latency.

For the callback conversion, grouping self costs across source files (including
inlined headers) gives:

| Work | Point instructions/feature | Point share | Line16 share |
|---|---:|---:|---:|
| Reader iteration, validity test, view setup | 40.65 | 8.6% | 5.4% |
| Callback adapter body | 6.00 | 1.3% | 0.7% |
| Generic writer validation and dispatch | 16.00 | 3.4% | 1.8% |
| Native append planning, checks, offsets, validity | 248.31 | 52.4% | 28.7% |
| Coordinate-copy setup and copying | 146.25 | 30.9% | 40.9% |
| Native append entry wrappers | 4.12 | 0.9% | 0.5% |
| Other: buffer growth, finish/release, benchmark loop | 12.59 | 2.7% | 22.1% |
| Total | 473.92 | 100% | 100% |

The adapter row counts its body; the reader row includes the call instruction.
Neither is a standalone measurement of the hardware cost of an indirect call.

The native append control row includes `GeoArrowNativeWriterCountCoords()` and
`GeoArrowNativeWriterPlanConversion()`, inlined into
`GeoArrowNativeWriterAppendInternal()`. Even a single point checks node dimensions
and coordinate pointers, counts storage, checks compatibility, initializes an
append plan, and maintains the validity bitmap. After the first null, validity
work also runs for subsequent non-null features.

`GeoArrowNativeWriterAppendCoordsUnsafe()` builds output pointer/stride arrays
and calls `GeoArrowGeometryViewCopyCoordsGeneric()`. The latter remaps dimensions,
loads source strides, tests byte order, and loops over nodes/dimensions/coordinates.
This setup is substantial when the payload is just two doubles. The event writer
uses its type-specific callbacks and `GeoArrowCoordViewCopy()` instead.

For lines, output-buffer growth is also visible: bulk `memcpy` accounts for about
177 instructions/feature in both the callback and event profiles. It is shared
work, not the explanation for the difference between paths. Node-list allocation
is absent from the steady point/line callback path. Scalar reads reuse capacity
but still execute node resizing/resetting and metadata setup on every feature.

Total instructions per feature are 268 / 642 / 474 for point event / scalar /
callback, and 581 / 1075 / 907 for Line16. See
[the instruction CSV](array_geometry_profile_instructions.csv).

The strongest optimization candidates are a matching-type point/sequence append
fast path and a coordinate-copy fast path for matching dimensions, byte order,
and contiguous strides. They could avoid rebuilding a general conversion plan
and dimension mapping for each feature while retaining the existing general
fallback. Further callback-interface changes or node allocation changes are
unlikely to address the measured small-geometry bottleneck by themselves.

## Reproduce

Measured 2026-11-09 on the same Ryzen 9 5900X / GCC 15.2 host as the conversion
report. Build with optimization and debug symbols; no inlining was disabled:

```sh
cmake -S dev/benchmarks -B build/bench-profile -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS_RELEASE='-O3 -g -DNDEBUG' \
  -DCMAKE_CXX_FLAGS_RELEASE='-O3 -g -DNDEBUG' \
  -DGEOARROW_BENCHMARK_SOURCE_URL="$PWD"
cmake --build build/bench-profile --target array_geometry_benchmark -j

taskset -c 2 build/bench-profile/array_geometry_benchmark \
  --benchmark_filter='^(profile/|(point|line16|polygon128|multipolygon16x5)/native_to_native/)' \
  --benchmark_min_time=0.15s --benchmark_repetitions=7 \
  --benchmark_enable_random_interleaving=true \
  --benchmark_display_aggregates_only=true \
  --benchmark_out=feature-costs.json
```

CPU scaling/boost remained enabled. The component timing coefficients of
variation range from 0.08% to 17.1%; the largest is the polygon void-visitor read.
The point and Line16 full conversions range from 0.3% to 2.7%. Timing runs were
separate from Callgrind runs.

To reproduce instruction counts:

```sh
for shape in point line16; do
  for strategy in visitor geometry callback; do
    for batches in 1 257; do
      name="$shape-$strategy-$batches"
      valgrind --tool=callgrind --collect-atstart=no \
        '--toggle-collect=*BenchConvert*' \
        --callgrind-out-file="$name.callgrind" \
        build/bench-profile/array_geometry_benchmark \
        --benchmark_filter="^$shape/native_to_native/$strategy$" \
        --benchmark_min_time="${batches}x"
      callgrind_annotate --auto=no --inclusive=no --threshold=100 \
        "$name.callgrind" > "$name.txt"
    done
  done
done
```

Subtract the one-batch self counts from the 257-batch counts, then divide by
256 times the batch size (4096 points or 1024 lines). This removes input creation,
correctness checks, initialization, and warm-up, while retaining the repeated
conversion, output growth, finish, and release. Group functions across their
source files, so inlined header costs are included. Annotating `native_writer.c`,
`geoarrow_type_inline.h`, and `nanoarrow.h` provides the source-level breakdown.
Allocator state can cause small variations between runs.

All 24 new component cases completed their output checks. A one-iteration run of
those cases under Memcheck reported zero errors and no lost allocations. The
library itself is unchanged from the previously tested draft PR.
