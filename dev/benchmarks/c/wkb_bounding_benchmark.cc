

#include <cstdint>
#include <iostream>
#include <sstream>

#include <benchmark/benchmark.h>

#include "hpp/array_util.hpp"
#include "hpp/binary_util.hpp"

#include "benchmark_util.hpp"
#include "geometry_util_internal.hpp"

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

bool AllNaN(XY coord) {
  int sum_na = 0;
  for (const double x : coord) {
    sum_na += std::isnan(x);
  }
  return sum_na == coord.size();
}

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

BoxXY BoundDoublesWithEmptyCheck(const std::vector<double>& coords) {
  BoxXY bounds = BoxXY::Empty();
  XY xy;
  for (size_t i = 0; i < coords.size(); i += 2) {
    xy = {coords[i], coords[i + 1]};
    if (!AllNaN(xy)) {
      bounds[0] = MIN(bounds[0], coords[i]);
      bounds[1] = MIN(bounds[1], coords[i + 1]);
      bounds[2] = MAX(bounds[2], coords[i]);
      bounds[3] = MAX(bounds[3], coords[i + 1]);
    }
  }

  return bounds;
}

void BenchBoundDoublesWithEmptyCheck(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundDoublesWithEmptyCheck(coords);
    benchmark::DoNotOptimize(bounds);
  }

  state.SetItemsProcessed(kNumCoordsPrettyBig * state.iterations());
  CheckResult(bounds);
}

BoxXY BoundWKBLinestringUsingGeoArrowHpp(const std::vector<uint8_t>& wkb) {
  BoxXY bounds = BoxXY::Empty();

  geoarrow::binary_util::WKBGeometry geometry;
  geoarrow::binary_util::WKBParser parser;
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

BoxXY BoundWKBLinestringUsingParquetBounder(const std::vector<uint8_t>& wkb) {
  parquet::geometry::WKBBuffer buffer;
  parquet::geometry::WKBGeometryBounder bounder;

  buffer.Init(wkb.data(), wkb.size());
  bounder.ReadGeometry(&buffer);
  bounder.Flush();
  auto result = bounder.Bounds();
  return {result.min[0], result.min[1], result.max[0], result.max[1]};
}

void BenchBoundWKBLinestringUsingParquetBounder(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakeLinestringWKB(coords);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBLinestringUsingParquetBounder(wkb);
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

  geoarrow::binary_util::WKBGeometry geometry;
  geoarrow::binary_util::WKBParser parser;

  for (uint32_t i = 0; i < num_points; i++) {
    if (parser.Parse(wkb.data() + (i * xy_point_bytes), xy_point_bytes, &geometry) !=
        parser.OK) {
      throw std::runtime_error("Error parsing WKB at index " + std::to_string(i));
    }

    geometry.VisitVertices<XY>([&](XY xy) {
      if (AllNaN(xy)) {
        return;
      }

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

BoxXY BoundWKBPointsUsingParquetBounder(const std::vector<uint8_t>& wkb,
                                        uint32_t num_points) {
  uint32_t xy_point_bytes = 21;

  parquet::geometry::WKBBuffer buffer;
  parquet::geometry::WKBGeometryBounder bounder;

  for (uint32_t i = 0; i < num_points; i++) {
    buffer.Init(wkb.data() + (i * xy_point_bytes), xy_point_bytes);
    bounder.ReadGeometry(&buffer);
  }

  bounder.Flush();
  auto result = bounder.Bounds();
  return {result.min[0], result.min[1], result.max[0], result.max[1]};
}

void BenchBoundWKBPointsUsingParquetBounder(benchmark::State& state) {
  std::vector<double> coords = MakeInterleavedCoords(kNumCoordsPrettyBig);
  std::vector<uint8_t> wkb = MakePointsWKB(coords);
  BoxXY bounds{};

  for (auto _ : state) {
    bounds = BoundWKBPointsUsingParquetBounder(wkb, kNumCoordsPrettyBig);
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

  geoarrow::binary_util::WKBGeometry geometry;
  geoarrow::binary_util::WKBParser parser;

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
    if (AllNaN(xy)) {
      return;
    }

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
BENCHMARK(BenchBoundDoublesWithEmptyCheck);
BENCHMARK(BenchBoundWKBLinestringUsingGeoArrowHpp);
BENCHMARK(BenchBoundWKBLinestringUsingParquetBounder);
BENCHMARK(BenchBoundWKBPointsUsingGeoArrowHpp);
BENCHMARK(BenchBoundWKBPointsUsingParquetBounder);
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
