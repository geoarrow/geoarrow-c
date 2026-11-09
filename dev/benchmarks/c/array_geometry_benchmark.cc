#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "geoarrow/geoarrow.hpp"

namespace {

void Check(int code) {
  if (code != GEOARROW_OK) {
    throw std::runtime_error("GeoArrow error " + std::to_string(code));
  }
}

struct Geometry {
  GeoArrowGeometry value{};
  Geometry() { Check(GeoArrowGeometryInit(&value)); }
  ~Geometry() { GeoArrowGeometryReset(&value); }
};

// Time the C writer API directly. The throwing C++ Append() wrapper creates an
// error buffer per feature, which can dominate the small-geometry measurements.
struct Writer {
  GeoArrowArrayWriter value{};
  GeoArrowVisitor visitor{};
  explicit Writer(GeoArrowType type) {
    Check(GeoArrowArrayWriterInitFromType(&value, type));
    Check(GeoArrowArrayWriterInitVisitor(&value, &visitor));
  }
  ~Writer() { GeoArrowArrayWriterReset(&value); }
};

struct Shape {
  const char* name;
  GeoArrowGeometryType type;
  int coords;
  int parts;
  int64_t features;
};

const Shape kShapes[] = {
    {"point", GEOARROW_GEOMETRY_TYPE_POINT, 1, 1, 4096},
    {"line16", GEOARROW_GEOMETRY_TYPE_LINESTRING, 16, 1, 1024},
    {"line1024", GEOARROW_GEOMETRY_TYPE_LINESTRING, 1024, 1, 32},
    {"polygon128", GEOARROW_GEOMETRY_TYPE_POLYGON, 128, 1, 128},
    {"multipolygon16x5", GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, 5, 16, 256}};

enum Strategy { VISITOR, GEOMETRY, CALLBACK };
const char* kStrategyNames[] = {"visitor", "geometry", "callback"};

enum Encoding { NATIVE, INTERLEAVED, WKB, WKT };
const char* kEncodingNames[] = {"native", "interleaved", "wkb", "wkt"};

GeoArrowType Type(const Shape& shape, Encoding encoding) {
  switch (encoding) {
    case WKB:
      return GEOARROW_TYPE_WKB;
    case WKT:
      return GEOARROW_TYPE_WKT;
    default:
      return GeoArrowMakeType(shape.type, GEOARROW_DIMENSIONS_XY,
                              encoding == NATIVE ? GEOARROW_COORD_TYPE_SEPARATE
                                                 : GEOARROW_COORD_TYPE_INTERLEAVED);
  }
}

std::string MakeWKT(const Shape& shape) {
  std::ostringstream text;
  text.precision(15);
  text << GeoArrowGeometryTypeString(shape.type) << " (";
  for (int part = 0; part < shape.parts; part++) {
    if (part > 0) text << ", ";
    if (shape.type == GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON) text << '(';
    if (shape.type == GEOARROW_GEOMETRY_TYPE_POLYGON ||
        shape.type == GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON)
      text << '(';
    for (int i = 0; i < shape.coords; i++) {
      if (i > 0) text << ", ";
      double angle = shape.coords == 1 ? 0 : 6.283185307179586 * i / (shape.coords - 1);
      if (i == shape.coords - 1 && (shape.type == GEOARROW_GEOMETRY_TYPE_POLYGON ||
                                    shape.type == GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON)) {
        angle = 0;
      }
      text << part * 3 + std::cos(angle) << ' ' << std::sin(angle);
    }
    if (shape.type == GEOARROW_GEOMETRY_TYPE_POLYGON ||
        shape.type == GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON)
      text << ')';
    if (shape.type == GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON) text << ')';
  }
  text << ')';
  return text.str();
}

void MakeInput(const Shape& shape, GeoArrowType type, ArrowArray* out) {
  Geometry geom;
  GeoArrowVisitor visitor{};
  GeoArrowGeometryInitVisitor(&geom.value, &visitor);
  GeoArrowWKTReader parser;
  Check(GeoArrowWKTReaderInit(&parser));
  auto text = MakeWKT(shape);
  int result = GeoArrowWKTReaderVisit(
      &parser, {text.data(), static_cast<int64_t>(text.size())}, &visitor);
  GeoArrowWKTReaderReset(&parser);
  Check(result);
  geoarrow::ArrayWriter writer(type);
  for (int64_t i = 0; i < shape.features; i++) {
    if (i % 16 == 15) {
      writer.AppendNull();
    } else {
      writer.Append(GeoArrowGeometryAsView(&geom.value));
    }
  }
  writer.Finish(out);
}

void Convert(geoarrow::ArrayReader& reader, Writer& writer, Geometry& geom,
             int64_t length, Strategy strategy, ArrowArray* out) {
  if (strategy == GEOMETRY) {
    for (int64_t i = 0; i < length; i++) {
      Check(reader.Read(i, &geom.value));
      Check(GeoArrowArrayWriterAppend(&writer.value, GeoArrowGeometryAsView(&geom.value),
                                      nullptr));
    }
  } else if (strategy == CALLBACK) {
    Check(reader.VisitGeometry(
        [](GeoArrowGeometryView value, void* data, GeoArrowError* error) {
          return GeoArrowArrayWriterAppend(static_cast<GeoArrowArrayWriter*>(data), value,
                                           error);
        },
        &writer.value, 0, length));
  } else {
    Check(reader.Visit(&writer.visitor, 0, length));
  }
  Check(GeoArrowArrayWriterFinish(&writer.value, out, nullptr));
}

void Canonicalize(const ArrowArray* array, GeoArrowType type, ArrowArray* out) {
  geoarrow::ArrayReader reader(type);
  reader.SetArrayNonOwning(array);
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_WKB);
  Check(reader.Visit(writer.visitor(), 0, array->length));
  writer.Finish(out);
}

// Compare both paths outside timing using the established visitor decoder.
void CheckEquivalent(const ArrowArray* left, const ArrowArray* right, GeoArrowType type) {
  geoarrow::internal::ArrayHolder a, b;
  Canonicalize(left, type, &a.array);
  Canonicalize(right, type, &b.array);
  if (a.array.length != b.array.length || a.array.null_count != b.array.null_count) {
    throw std::runtime_error("Output lengths or null counts differ");
  }
  auto offsets_a = static_cast<const int32_t*>(a.array.buffers[1]);
  auto offsets_b = static_cast<const int32_t*>(b.array.buffers[1]);
  if (std::memcmp(offsets_a, offsets_b, (a.array.length + 1) * sizeof(int32_t)) != 0 ||
      std::memcmp(a.array.buffers[2], b.array.buffers[2], offsets_a[a.array.length]) !=
          0 ||
      (a.array.null_count > 0 && std::memcmp(a.array.buffers[0], b.array.buffers[0],
                                             (a.array.length + 7) / 8) != 0)) {
    throw std::runtime_error("Output geometries differ");
  }
}

void BenchConvert(benchmark::State& state, Shape shape, Encoding source,
                  Encoding destination, Strategy strategy) {
  auto src_type = Type(shape, source);
  auto dst_type = Type(shape, destination);
  geoarrow::internal::ArrayHolder input;
  MakeInput(shape, src_type, &input.array);
  geoarrow::ArrayReader reader(src_type);
  reader.SetArrayNonOwning(&input.array);
  Writer writer(dst_type);
  Geometry geom;
  {
    geoarrow::internal::ArrayHolder direct, visited, callback;
    Convert(reader, writer, geom, shape.features, GEOMETRY, &direct.array);
    Convert(reader, writer, geom, shape.features, VISITOR, &visited.array);
    Convert(reader, writer, geom, shape.features, CALLBACK, &callback.array);
    CheckEquivalent(&direct.array, &visited.array, dst_type);
    CheckEquivalent(&callback.array, &visited.array, dst_type);
  }

  // Input creation, reader/writer initialization, and validation are excluded.
  // Feature iteration, output allocation, Finish(), and release are included.
  for (auto _ : state) {
    geoarrow::internal::ArrayHolder output;
    Convert(reader, writer, geom, shape.features, strategy, &output.array);
    benchmark::DoNotOptimize(output.array.length);
  }
  state.SetItemsProcessed(state.iterations() * shape.features);
  state.counters["coords_per_batch"] =
      static_cast<double>(shape.features * 15 / 16 * shape.coords * shape.parts);
}

bool RegisterBenchmarks() {
  for (const auto& shape : kShapes) {
    for (auto source : {NATIVE, WKB, WKT}) {
      for (auto destination : {NATIVE, WKB, WKT}) {
        for (auto strategy : {VISITOR, GEOMETRY, CALLBACK}) {
          std::string name = std::string(shape.name) + "/" + kEncodingNames[source] +
                             "_to_" + kEncodingNames[destination] + std::string("/") +
                             kStrategyNames[strategy];
          benchmark::RegisterBenchmark(name.c_str(), BenchConvert, shape, source,
                                       destination, strategy);
        }
      }
    }
    for (auto strategy : {VISITOR, GEOMETRY, CALLBACK}) {
      for (auto source : {INTERLEAVED, WKB}) {
        auto destination = source == WKB ? INTERLEAVED : WKB;
        std::string name = std::string(shape.name) + "/" + kEncodingNames[source] +
                           "_to_" + kEncodingNames[destination] + std::string("/") +
                           kStrategyNames[strategy];
        benchmark::RegisterBenchmark(name.c_str(), BenchConvert, shape, source,
                                     destination, strategy);
      }
    }
  }
  return true;
}

// Component measurements keep the same native input and null frequency as the
// end-to-end conversions. These are diagnostic timings, not additive costs:
// removing a consumer changes register pressure, cache use, and branch behavior.
void BenchReadOnly(benchmark::State& state, Shape shape, Strategy strategy) {
  geoarrow::internal::ArrayHolder input;
  MakeInput(shape, Type(shape, NATIVE), &input.array);
  geoarrow::ArrayReader reader(Type(shape, NATIVE));
  reader.SetArrayNonOwning(&input.array);
  Geometry geom;
  GeoArrowVisitor visitor{};
  GeoArrowVisitorInitVoid(&visitor);
  auto callback = [](GeoArrowGeometryView value, void*, GeoArrowError*) {
    benchmark::DoNotOptimize(value);
    return GEOARROW_OK;
  };
  // Warm scratch storage before timing multipart reads.
  Check(reader.Read(0, &geom.value));
  Check(reader.VisitGeometry(callback, nullptr, 0, shape.features));
  for (auto _ : state) {
    if (strategy == GEOMETRY) {
      for (int64_t i = 0; i < shape.features; i++) {
        Check(reader.Read(i, &geom.value));
        benchmark::DoNotOptimize(geom.value.root);
      }
    } else if (strategy == CALLBACK) {
      Check(reader.VisitGeometry(callback, nullptr, 0, shape.features));
    } else {
      Check(reader.Visit(&visitor, 0, shape.features));
    }
  }
  state.SetItemsProcessed(state.iterations() * shape.features);
}

struct NativeWriter {
  GeoArrowNativeWriter value{};
  explicit NativeWriter(GeoArrowType type) {
    Check(GeoArrowNativeWriterInit(&value, type));
  }
  ~NativeWriter() { GeoArrowNativeWriterReset(&value); }
};

GeoArrowErrorCode AppendNative(GeoArrowGeometryView geom, void* data,
                               GeoArrowError* error) {
  auto writer = static_cast<GeoArrowNativeWriter*>(data);
  if (geom.size_nodes == 1 &&
      geom.root->geometry_type == GEOARROW_GEOMETRY_TYPE_GEOMETRY) {
    return GeoArrowNativeWriterAppendNull(writer);
  }
  return GeoArrowNativeWriterAppend(writer, geom, error);
}

void BenchWriteComponent(benchmark::State& state, Shape shape, bool cached,
                         bool generic) {
  auto type = Type(shape, NATIVE);
  geoarrow::internal::ArrayHolder input;
  MakeInput(shape, type, &input.array);
  geoarrow::ArrayReader reader(type);
  reader.SetArrayNonOwning(&input.array);
  Writer writer(type);
  NativeWriter native_writer(type);
  Geometry geom;
  std::vector<std::vector<GeoArrowGeometryNode>> nodes;
  std::vector<GeoArrowGeometryView> views;
  if (cached) {
    nodes.reserve(shape.features);
    views.reserve(shape.features);
    for (int64_t i = 0; i < shape.features; i++) {
      Check(reader.Read(i, &geom.value));
      auto view = GeoArrowGeometryAsView(&geom.value);
      nodes.emplace_back(view.root, view.root + view.size_nodes);
      views.push_back({nodes.back().data(), view.size_nodes});
    }
  }
  auto convert = [&](ArrowArray* out) {
    if (cached) {
      for (auto view : views) {
        Check(generic ? GeoArrowArrayWriterAppend(&writer.value, view, nullptr)
                      : AppendNative(view, &native_writer.value, nullptr));
      }
    } else {
      Check(reader.VisitGeometry(AppendNative, &native_writer.value, 0, shape.features));
    }
    Check(generic ? GeoArrowArrayWriterFinish(&writer.value, out, nullptr)
                  : GeoArrowNativeWriterFinish(&native_writer.value, out, nullptr));
  };
  {
    geoarrow::internal::ArrayHolder output;
    convert(&output.array);
    CheckEquivalent(&input.array, &output.array, type);
  }
  for (auto _ : state) {
    geoarrow::internal::ArrayHolder output;
    convert(&output.array);
    benchmark::DoNotOptimize(output.array.length);
  }
  state.SetItemsProcessed(state.iterations() * shape.features);
}

bool RegisterComponentBenchmarks() {
  for (int index : {0, 1, 3, 4}) {
    const auto& shape = kShapes[index];
    std::string prefix = std::string("profile/") + shape.name;
    for (auto strategy : {VISITOR, GEOMETRY, CALLBACK}) {
      std::string name = prefix + "/read_only/" + kStrategyNames[strategy];
      benchmark::RegisterBenchmark(name.c_str(), BenchReadOnly, shape, strategy);
    }
    for (bool generic : {false, true}) {
      std::string name = prefix + "/cached_write/" + (generic ? "generic" : "native");
      benchmark::RegisterBenchmark(name.c_str(), BenchWriteComponent, shape, true,
                                   generic);
    }
    std::string name = prefix + "/native_callback";
    benchmark::RegisterBenchmark(name.c_str(), BenchWriteComponent, shape, false, false);
  }
  return true;
}

const bool kComponentsRegistered = RegisterComponentBenchmarks();

const bool kRegistered = RegisterBenchmarks();
}  // namespace
