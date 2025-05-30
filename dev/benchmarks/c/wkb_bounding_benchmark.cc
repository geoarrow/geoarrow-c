
#include <cstdint>
#include <iostream>
#include <sstream>

#include <benchmark/benchmark.h>

#include "geoarrow/geoarrow.hpp"

#include "benchmark_util.hpp"

/// \file wkb_bounding_benchmark.cc
///
/// Benchmarks related to calculating bounds. Three situations with an identical
/// coordinate count are considered:
///
/// - A raw (aligned) double array. This should be the fastest/simplest.
/// - A single linestring (where parsing overhead should be minimal)
/// - All coordinates as their own WKB points. Here, parsing overhead should
///   dominate

// Seems to be slightly faster than std::min/std::max
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

using BoxXY = geoarrow::array_util::BoxXY<double>;
using XY = geoarrow::array_util::XY<double>;
using geoarrow::benchmark_util::kNumCoordsPrettyBig;

std::vector<double> MakeInterleavedCoords(uint32_t num_coords);
std::vector<uint8_t> MakePointsWKB(const std::vector<double>& coords);
std::vector<uint8_t> MakeLinestringWKB(const std::vector<double>& coords);

// Coarse check to make sure we're benchmarking the same thing
void CheckResult(const BoxXY& bounds) {
  std::string expected = "BoxXY{123, 10, 456, 20}";
  std::stringstream ss;
  ss << "BoxXY{" << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
     << bounds[3] << "}";
  if (ss.str() != expected) {
    throw std::runtime_error("Expected " + expected + " but got " + ss.str());
  }
}

BoxXY BoundDoubles(const std::vector<double>& coords) {
  BoxXY bounds = BoxXY::Empty();

  XY xy;
  for (size_t i = 0; i < coords.size(); i += 2) {
    xy = {coords[i], coords[i + 1]};
    bounds[0] = MIN(bounds[0], xy.x());
    bounds[1] = MIN(bounds[1], xy.y());
    bounds[2] = MAX(bounds[2], xy.x());
    bounds[3] = MAX(bounds[3], xy.y());
  }

  return bounds;
}

void BenchBoundDoubles(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundDoubles(coords);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

void BenchBoundGeometry(struct GeoArrowGeometryView geom, BoxXY* bounds) {
  const struct GeoArrowGeometryNode* node;
  const struct GeoArrowGeometryNode* end;
  const uint8_t* px;
  const uint8_t* py;
  int32_t dx, dy;
  double x, y;

  end = geom.root + geom.size_nodes;
  for (node = geom.root; node < end; node++) {
    switch (node->geometry_type) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
        px = geom.root->coords[0];
        py = geom.root->coords[1];
        dx = geom.root->coord_stride[0];
        dy = geom.root->coord_stride[1];

        if (node->flags & GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN) {
          throw std::runtime_error("big endian not supported");
        }

        for (uint32_t i = 0; i < node->size; i++) {
          std::memcpy(&x, px, sizeof(double));
          std::memcpy(&y, py, sizeof(double));

          bounds->at(0) = MIN(bounds->at(0), x);
          bounds->at(1) = MIN(bounds->at(1), y);
          bounds->at(2) = MAX(bounds->at(2), x);
          bounds->at(3) = MAX(bounds->at(3), y);

          px += dx;
          py += dy;
        }
        break;
      default:
        break;
    }
  }
}

BoxXY BoundWKBLinestringUsingGeoArrowHpp(const std::vector<uint8_t>& wkb) {
  BoxXY bounds = BoxXY::Empty();

  geoarrow::wkb_util::WKBGeometry geometry;
  geoarrow::wkb_util::WKBParser parser;
  if (parser.Parse(wkb.data(), wkb.size(), &geometry) != parser.OK) {
    throw std::runtime_error("Error parsing WKB");
  }

  geometry.VisitVertices<XY>([&](XY xy) {
    bounds[0] = MIN(bounds[0], xy.x());
    bounds[1] = MIN(bounds[1], xy.y());
    bounds[2] = MAX(bounds[2], xy.x());
    bounds[3] = MAX(bounds[3], xy.y());
  });

  return bounds;
}

void BenchBoundWKBLinestringUsingGeoArrowHpp(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakeLinestringWKB(coords);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBLinestringUsingGeoArrowHpp(wkb);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

BoxXY BoundWKBPointsUsingGeoArrowHpp(const std::vector<uint8_t>& wkb,
                                     uint32_t num_points) {
  uint32_t xy_point_bytes = 21;
  if (wkb.size() != (num_points * xy_point_bytes)) {
    throw std::runtime_error("Expected " + std::to_string(num_points * xy_point_bytes) +
                             " bytes but got " + std::to_string(wkb.size()));
  }

  BoxXY bounds = BoxXY::Empty();

  geoarrow::wkb_util::WKBGeometry geometry;
  geoarrow::wkb_util::WKBParser parser;

  for (uint32_t i = 0; i < num_points; i++) {
    if (parser.Parse(wkb.data() + (i * xy_point_bytes), xy_point_bytes, &geometry) !=
        parser.OK) {
      throw std::runtime_error("Error parsing WKB at index " + std::to_string(i));
    }

    geometry.VisitVertices<XY>([&](XY xy) {
      bounds[0] = MIN(bounds[0], xy.x());
      bounds[1] = MIN(bounds[1], xy.y());
      bounds[2] = MAX(bounds[2], xy.x());
      bounds[3] = MAX(bounds[3], xy.y());
    });
  }

  return bounds;
}

void BenchBoundWKBPointsUsingGeoArrowHpp(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakePointsWKB(coords);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBPointsUsingGeoArrowHpp(wkb, kNumCoordsPrettyBig);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

BoxXY BoundWKBLinestringUsingWkbReader(const std::vector<uint8_t>& wkb) {
  BoxXY bounds = BoxXY::Empty();

  struct GeoArrowWKBReader reader;
  GeoArrowWKBReaderInit(&reader);

  struct GeoArrowBufferView src;
  src.data = wkb.data();
  src.size_bytes = wkb.size();

  struct GeoArrowGeometryView geom;
  if (GeoArrowWKBReaderRead(&reader, src, &geom, nullptr) != GEOARROW_OK) {
    throw std::runtime_error("Error parsing WKB");
  }

  BenchBoundGeometry(geom, &bounds);

  GeoArrowWKBReaderReset(&reader);
  return bounds;
}

void BenchBoundWKBLinestringUsingWkbReader(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakeLinestringWKB(coords);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBLinestringUsingWkbReader(wkb);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

BoxXY BoundWKBPointsUsingWkbReader(const std::vector<uint8_t>& wkb, uint32_t num_points) {
  uint32_t xy_point_bytes = 21;
  if (wkb.size() != (num_points * xy_point_bytes)) {
    throw std::runtime_error("Expected " + std::to_string(num_points * xy_point_bytes) +
                             " bytes but got " + std::to_string(wkb.size()));
  }

  BoxXY bounds = BoxXY::Empty();

  struct GeoArrowWKBReader reader;
  GeoArrowWKBReaderInit(&reader);

  struct GeoArrowBufferView src;
  struct GeoArrowGeometryView geom;
  const struct GeoArrowGeometryNode* node;
  const struct GeoArrowGeometryNode* end;
  const uint8_t* px;
  const uint8_t* py;
  int32_t dx, dy;
  double x, y;

  for (uint32_t i = 0; i < num_points; i++) {
    src.data = wkb.data() + (i * xy_point_bytes);
    src.size_bytes = xy_point_bytes;
    if (GeoArrowWKBReaderRead(&reader, src, &geom, nullptr) != GEOARROW_OK) {
      throw std::runtime_error("Error parsing WKB");
    }

    end = geom.root + geom.size_nodes;
    for (node = geom.root; node < end; node++) {
      switch (node->geometry_type) {
        case GEOARROW_GEOMETRY_TYPE_POINT:
        case GEOARROW_GEOMETRY_TYPE_LINESTRING:
          px = geom.root->coords[0];
          py = geom.root->coords[1];
          dx = geom.root->coord_stride[0];
          dy = geom.root->coord_stride[1];

          if (node->flags & GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN) {
            throw std::runtime_error("big endian not supported");
          }

          for (uint32_t i = 0; i < node->size; i++) {
            std::memcpy(&x, px, sizeof(double));
            std::memcpy(&y, py, sizeof(double));

            bounds[0] = MIN(bounds[0], x);
            bounds[1] = MIN(bounds[1], y);
            bounds[2] = MAX(bounds[2], x);
            bounds[3] = MAX(bounds[3], y);

            px += dx;
            py += dy;
          }
          break;
        default:
          break;
      }
    }
  }

  GeoArrowWKBReaderReset(&reader);
  return bounds;
}

void BenchBoundWKBPointsUsingWkbReader(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakePointsWKB(coords);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBPointsUsingWkbReader(wkb, kNumCoordsPrettyBig);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

struct VisitingBounder {
  BoxXY bounds;

  void InitVisitor(struct GeoArrowVisitor* v) {
    GeoArrowVisitorInitVoid(v);
    v->coords = [](struct GeoArrowVisitor* v,
                   const struct GeoArrowCoordView* coords) -> int {
      auto self = reinterpret_cast<VisitingBounder*>(v->private_data);
      const double* x = coords->values[0];
      const double* y = coords->values[1];

      for (int64_t i = 0; i < coords->n_coords; i++) {
        self->bounds[0] = MIN(self->bounds[0], *x);
        self->bounds[1] = MIN(self->bounds[1], *y);
        self->bounds[2] = MAX(self->bounds[2], *x);
        self->bounds[3] = MAX(self->bounds[3], *y);
        x += coords->coords_stride;
        y += coords->coords_stride;
      }

      return GEOARROW_OK;
    };

    v->private_data = this;
  }
};

BoxXY BoundWKBUsingVisitor(geoarrow::ArrayReader* reader, int64_t length) {
  VisitingBounder bounder;
  bounder.bounds = BoxXY::Empty();

  struct GeoArrowVisitor v;
  bounder.InitVisitor(&v);
  if (reader->Visit(&v, 0, length) != GEOARROW_OK) {
    throw std::runtime_error("Visit() errored");
  }

  return bounder.bounds;
}

void BenchBoundWKBPointsUsingVisitor(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakePointsWKB(coords);

  int32_t offset = 0;
  std::vector<int32_t> offsets = {offset};
  for (int64_t i = 0; i < kNumCoordsPrettyBig; i++) {
    offset += 21;
    offsets.push_back(offset);
  }

  if (offset != wkb.size()) {
    throw std::runtime_error("Expected " + std::to_string(wkb.size()) +
                             " bytes but got " + std::to_string(offset));
  }

  geoarrow::ArrayBuilder builder(GEOARROW_TYPE_WKB);
  builder.SetBufferWrapped(1, offsets);
  builder.SetBufferWrapped(2, wkb);

  struct ArrowArray array;
  builder.Finish(&array);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array);

  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBUsingVisitor(&reader, kNumCoordsPrettyBig);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

void BenchBoundWKBLinestringUsingVisitor(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakeLinestringWKB(coords);

  std::vector<int32_t> offsets = {0, static_cast<int32_t>(wkb.size())};
  geoarrow::ArrayBuilder builder(GEOARROW_TYPE_WKB);
  builder.SetBufferWrapped(1, offsets);
  builder.SetBufferWrapped(2, wkb);

  struct ArrowArray array;
  builder.Finish(&array);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array);

  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBUsingVisitor(&reader, 1);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

BoxXY BoundWKBPointsUsingUnalignedSequence(const std::vector<uint8_t>& wkb,
                                           uint32_t num_points) {
  uint32_t xy_point_bytes = 21;
  if (wkb.size() != (num_points * xy_point_bytes)) {
    throw std::runtime_error("Expected " + std::to_string(num_points * xy_point_bytes) +
                             " bytes but got " + std::to_string(wkb.size()));
  }

  BoxXY bounds = BoxXY::Empty();

  uint32_t endian_bytes_le = 0;
  for (uint32_t i = 0; i < num_points; i++) {
    endian_bytes_le += 0x01 == wkb[i * xy_point_bytes];
  }

  if (endian_bytes_le != num_points) {
    throw std::runtime_error("not all endian bytes are 0x01");
  }

  uint32_t geometry_type_bytes_point = 0;
  uint32_t geometry_type;
  for (uint32_t i = 0; i < num_points; i++) {
    std::memcpy(&geometry_type, wkb.data() + (i * xy_point_bytes) + sizeof(uint8_t),
                sizeof(geometry_type));
    geometry_type_bytes_point += GEOARROW_GEOMETRY_TYPE_POINT == geometry_type;
  }

  if (geometry_type_bytes_point != num_points) {
    throw std::runtime_error("not all geometry types are 0x01000000");
  }

  geoarrow::array_util::UnalignedCoordSequence<XY> seq;
  seq.InitInterleaved(num_points, wkb.data() + sizeof(uint8_t) + sizeof(uint32_t));
  seq.stride_bytes = xy_point_bytes;
  seq.VisitVertices<XY>([&](XY xy) {
    bounds[0] = MIN(bounds[0], xy.x());
    bounds[1] = MIN(bounds[1], xy.y());
    bounds[2] = MAX(bounds[2], xy.x());
    bounds[3] = MAX(bounds[3], xy.y());
  });

  return bounds;
}

void BenchBoundWKBPointsUsingUnalignedSequence(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakePointsWKB(coords);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBPointsUsingUnalignedSequence(wkb, kNumCoordsPrettyBig);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

BENCHMARK(BenchBoundDoubles);
BENCHMARK(BenchBoundWKBLinestringUsingGeoArrowHpp);
BENCHMARK(BenchBoundWKBPointsUsingGeoArrowHpp);
BENCHMARK(BenchBoundWKBLinestringUsingWkbReader);
BENCHMARK(BenchBoundWKBPointsUsingWkbReader);
BENCHMARK(BenchBoundWKBPointsUsingVisitor);
BENCHMARK(BenchBoundWKBLinestringUsingVisitor);
BENCHMARK(BenchBoundWKBPointsUsingUnalignedSequence);

std::vector<double> MakeInterleavedCoords(uint32_t num_coords) {
  std::vector<double> coords(num_coords * 2);
  geoarrow::benchmark_util::FillRandom(num_coords, 2, coords.data(), 1234, 123.0, 456.0);
  geoarrow::benchmark_util::FillRandom(num_coords, 2, coords.data() + 1, 5678, 10.0,
                                       20.0);
  return coords;
}

// Make one really long linestring (i.e. minimal parsing overhead but a lot of coordinates
// to bound)
std::vector<uint8_t> MakeLinestringWKB(const std::vector<double>& coords) {
  uint32_t num_coords = coords.size() / 2;
  std::vector<uint8_t> out(sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                           (num_coords * 2 * sizeof(double)));

  uint8_t* cursor = out.data();
  *cursor = 0x01;
  ++cursor;

  uint32_t geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
  std::memcpy(cursor, &geometry_type, sizeof(geometry_type));
  cursor += sizeof(geometry_type);

  std::memcpy(cursor, &num_coords, sizeof(num_coords));
  cursor += sizeof(num_coords);

  std::memcpy(cursor, coords.data(), coords.size() * sizeof(double));
  cursor += coords.size() * sizeof(double);

  return out;
}

std::vector<uint8_t> MakePointsWKB(const std::vector<double>& coords) {
  uint32_t num_coords = coords.size() / 2;
  size_t item_size = sizeof(uint8_t) + sizeof(uint32_t) + (2 * sizeof(double));
  uint32_t geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;

  std::vector<uint8_t> out(item_size * num_coords);
  uint8_t* cursor = out.data();
  for (uint32_t i = 0; i < num_coords; ++i) {
    *cursor = 0x01;
    ++cursor;

    std::memcpy(cursor, &geometry_type, sizeof(geometry_type));
    cursor += sizeof(geometry_type);

    std::memcpy(cursor, coords.data() + (2 * i), 2 * sizeof(double));
    cursor += 2 * sizeof(double);
  }

  return out;
}
