// From the repository root:
// cmake -S dev/benchmarks -B out/build/benchmarks -DCMAKE_BUILD_TYPE=Release
//       -DGEOARROW_BENCHMARK_SOURCE_URL="$PWD"
// cmake --build out/build/benchmarks --target builder_geometry_benchmark -j
// out/build/benchmarks/builder_geometry_benchmark --benchmark_min_time=0.05s
//       --benchmark_repetitions=5 --benchmark_enable_random_interleaving=true
//       --benchmark_out=out/build/benchmarks/builder_geometry.json
// Each command above is one line. Optional Linux taskset pins the benchmark to
// one CPU. Timings are per 64-feature batch; divide by 64 for time per feature.
// Fixtures are non-null/non-empty, with matching source/output layout and dimensions.

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "geoarrow/geoarrow.h"

namespace {

// Match the fixtures used by array_view_geometry_benchmark: 64 distinct
// features, four rings/parts per container, and short or long sequences.
struct GeometryFixture {
  static constexpr int64_t kNumFeatures = 64;
  struct GeoArrowArrayView view;
  std::vector<int32_t> offsets[3];
  std::vector<double> coords;

  GeometryFixture(enum GeoArrowType type, int32_t sequence_size) {
    if (GeoArrowArrayViewInitFromType(&view, type) != GEOARROW_OK) {
      throw std::runtime_error("Failed to initialize array view");
    }

    int64_t size = kNumFeatures;
    for (int level = 0; level < view.n_offsets; level++) {
      view.length[level] = size;
      // Four rings/parts per container; sequence_size coordinates per sequence.
      int32_t child_size = level == view.n_offsets - 1 ? sequence_size : 4;
      offsets[level].resize(size + 1);
      for (int64_t i = 0; i <= size; i++) {
        offsets[level][i] = static_cast<int32_t>(i * child_size);
      }
      view.offsets[level] = offsets[level].data();
      view.first_offset[level] = 0;
      view.last_offset[level] = offsets[level].back();
      size *= child_size;
    }

    view.length[view.n_offsets] = size;
    view.coords.n_coords = size;
    coords.resize(size * view.coords.n_values);
    for (int j = 0; j < view.coords.n_values; j++) {
      int64_t offset =
          view.schema_view.coord_type == GEOARROW_COORD_TYPE_SEPARATE ? j * size : j;
      view.coords.values[j] = coords.data() + offset;
      for (int64_t i = 0; i < size; i++) {
        coords[offset + i * view.coords.coords_stride] = static_cast<double>(i + j);
      }
    }
  }
};

struct Geometry {
  struct GeoArrowGeometry value;
  Geometry() {
    if (GeoArrowGeometryInit(&value) != GEOARROW_OK) {
      throw std::runtime_error("Failed to initialize geometry");
    }
  }
  ~Geometry() { GeoArrowGeometryReset(&value); }
};

void Check(int code) {
  if (code != GEOARROW_OK) {
    throw std::runtime_error("GeoArrow call failed: " + std::to_string(code));
  }
}

struct Array {
  struct ArrowArray value = {};
  ~Array() {
    if (value.release != nullptr) {
      value.release(&value);
    }
  }
};

enum class Strategy { Builder, Visitor, NativeWriter };

template <Strategy strategy>
struct BatchBuilder {
  struct GeoArrowBuilder builder = {};
  struct GeoArrowNativeWriter writer = {};
  struct GeoArrowVisitor visitor = {};

  explicit BatchBuilder(enum GeoArrowType type) {
    if constexpr (strategy == Strategy::Builder) {
      Check(GeoArrowBuilderInitFromType(&builder, type));
    } else {
      Check(GeoArrowNativeWriterInit(&writer, type));
      if constexpr (strategy == Strategy::Visitor) {
        Check(GeoArrowNativeWriterInitVisitor(&writer, &visitor));
      }
    }
  }

  ~BatchBuilder() {
    if constexpr (strategy == Strategy::Builder) {
      GeoArrowBuilderReset(&builder);
    } else {
      GeoArrowNativeWriterReset(&writer);
    }
  }

  void Append(struct GeoArrowGeometryView geom) {
    if constexpr (strategy == Strategy::Builder) {
      Check(GeoArrowBuilderAppendGeometry(&builder, geom, nullptr));
    } else if constexpr (strategy == Strategy::Visitor) {
      Check(GeoArrowGeometryViewVisit(geom, &visitor));
    } else {
      Check(GeoArrowNativeWriterAppend(&writer, geom, nullptr));
    }
  }

  void Finish(struct ArrowArray* array) {
    if constexpr (strategy == Strategy::Builder) {
      Check(GeoArrowBuilderFinish(&builder, array, nullptr));
    } else {
      Check(GeoArrowNativeWriterFinish(&writer, array, nullptr));
    }
  }
};

// Validate every offset and coordinate outside the timed loop. Each strategy
// must reproduce the source array, not just a plausible length or node count.
void CheckArray(const GeometryFixture& fixture, const struct ArrowArray* array) {
  if (array->length != GeometryFixture::kNumFeatures || array->null_count != 0) {
    throw std::runtime_error("Output length or null count differs");
  }
  struct GeoArrowArrayView actual;
  Check(GeoArrowArrayViewInitFromType(&actual, fixture.view.schema_view.type));
  Check(GeoArrowArrayViewSetArray(&actual, array, nullptr));
  for (int level = 0; level < actual.n_offsets; level++) {
    const auto& expected = fixture.offsets[level];
    if (actual.length[level] != static_cast<int64_t>(expected.size()) - 1 ||
        std::memcmp(actual.offsets[level], expected.data(),
                    expected.size() * sizeof(int32_t)) != 0) {
      throw std::runtime_error("Output offsets differ");
    }
  }
  if (actual.coords.n_coords != fixture.view.coords.n_coords) {
    throw std::runtime_error("Output coordinate count differs");
  }
  for (int j = 0; j < actual.coords.n_values; j++) {
    for (int64_t i = 0; i < actual.coords.n_coords; i++) {
      if (GEOARROW_COORD_VIEW_VALUE(&actual.coords, i, j) !=
          GEOARROW_COORD_VIEW_VALUE(&fixture.view.coords, i, j)) {
        throw std::runtime_error("Output coordinates differ");
      }
    }
  }
}

template <Strategy strategy>
void BuildGeometry(benchmark::State& state, enum GeoArrowType type,
                   int32_t sequence_size) {
  GeometryFixture fixture(type, sequence_size);
  // Extract the same borrowed scalar views once for every strategy. Neither
  // fixture construction nor GetGeometry() is included in the measured time.
  Geometry geometries[GeometryFixture::kNumFeatures];
  for (int64_t i = 0; i < GeometryFixture::kNumFeatures; i++) {
    Check(GeoArrowArrayViewGetGeometry(&fixture.view, i, &geometries[i].value));
  }
  BatchBuilder<strategy> builder(type);
  {
    Array validation;
    for (const auto& geom : geometries) {
      builder.Append(GeoArrowGeometryAsView(&geom.value));
    }
    builder.Finish(&validation.value);
    CheckArray(fixture, &validation.value);
  }

  // One iteration produces and releases one complete batch. Finish resets the
  // builder/writer for reuse; buffer growth, Finish, and release are all timed.
  for (auto _ : state) {
    Array output;
    for (const auto& geom : geometries) {
      builder.Append(GeoArrowGeometryAsView(&geom.value));
    }
    builder.Finish(&output.value);
    benchmark::DoNotOptimize(output.value.buffers);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * GeometryFixture::kNumFeatures);
  state.SetBytesProcessed(state.iterations() * fixture.view.coords.n_coords *
                          fixture.view.coords.n_values * sizeof(double));
}

bool RegisterBuilderGeometryBenchmarks() {
  const char* dimensions[] = {"", "XY", "XYZ", "XYM", "XYZM"};
  for (int geometry_type = 1; geometry_type <= 6; geometry_type++) {
    for (int dims = 1; dims <= 4; dims++) {
      for (auto layout :
           {GEOARROW_COORD_TYPE_SEPARATE, GEOARROW_COORD_TYPE_INTERLEAVED}) {
        for (int32_t sequence_size : {4, 1024}) {
          if (geometry_type == GEOARROW_GEOMETRY_TYPE_POINT && sequence_size != 4) {
            continue;
          }
          auto type = GeoArrowMakeType(static_cast<GeoArrowGeometryType>(geometry_type),
                                       static_cast<GeoArrowDimensions>(dims), layout);
          std::string suffix =
              std::string("/") +
              GeoArrowGeometryTypeString(
                  static_cast<GeoArrowGeometryType>(geometry_type)) +
              "/" + dimensions[dims] + "/" +
              (layout == GEOARROW_COORD_TYPE_SEPARATE ? "Separate" : "Interleaved") +
              "/" +
              std::to_string(
                  geometry_type == GEOARROW_GEOMETRY_TYPE_POINT ? 1 : sequence_size);
          benchmark::RegisterBenchmark(("BuilderAppendGeometry" + suffix).c_str(),
                                       &BuildGeometry<Strategy::Builder>, type,
                                       sequence_size);
          benchmark::RegisterBenchmark(("GeometryViewVisit" + suffix).c_str(),
                                       &BuildGeometry<Strategy::Visitor>, type,
                                       sequence_size);
          benchmark::RegisterBenchmark(("NativeWriterAppend" + suffix).c_str(),
                                       &BuildGeometry<Strategy::NativeWriter>, type,
                                       sequence_size);
        }
      }
    }
  }
  return true;
}

const bool kRegistered = RegisterBuilderGeometryBenchmarks();

}  // namespace
