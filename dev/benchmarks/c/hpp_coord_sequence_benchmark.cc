
#include <cstdint>

#include <benchmark/benchmark.h>

#include "hpp/array_util.hpp"

#include "benchmark_util.hpp"

using geoarrow::array_util::CoordSequence;
using geoarrow::array_util::UnalignedCoordSequence;
using XY = geoarrow::array_util::XY<double>;

enum Operation { BOUNDS, CENTROID };
enum Strategy { STL_ITERATOR, POINTERS };

// Slightly faster than std::min/std::max
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Calculates total bounds using the STL iterator
template <typename Sequence>
std::array<double, 4> BoundsUsingCoordIterator(const Sequence& seq) {
  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();

  for (const XY coord : seq) {
    xmin = MIN(xmin, coord.x());
    xmax = MAX(xmax, coord.x());
    ymin = MIN(ymin, coord.y());
    ymax = MAX(ymax, coord.y());
  }

  return {xmin, ymin, xmax, ymax};
}

// Calculates total bounds using raw pointer iteration
template <typename Sequence>
std::array<double, 4> BoundsUsingPointers(const Sequence& seq) {
  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();

  auto x = seq.dbegin(0);
  auto y = seq.dbegin(1);
  for (uint32_t i = 0; i < seq.size(); i++) {
    xmin = MIN(xmin, *x);
    xmax = MAX(xmax, *x);
    ++x;

    ymin = MIN(xmin, *y);
    ymax = MAX(xmax, *y);
    ++y;
  }

  return {xmin, ymin, xmax, ymax};
}

// Calculates centroid using the STL iterator
template <typename Sequence>
std::array<double, 2> CentroidUsingCoordIterator(const Sequence& seq) {
  double xsum = 0;
  double ysum = 0;

  for (const XY coord : seq) {
    xsum += coord.x();
    ysum += coord.y();
  }

  return {xsum / seq.size(), ysum / seq.size()};
}

// Calculates centroid using raw pointer iteration
template <typename Sequence>
std::array<double, 2> CentroidUsingPointers(const Sequence& seq) {
  double xsum = 0;
  double ysum = 0;

  auto x = seq.dbegin(0);
  auto y = seq.dbegin(1);
  for (uint32_t i = 0; i < seq.size(); i++) {
    xsum += x++;
    ysum += y++;
  }

  return {xsum / seq.size(), ysum / seq.size()};
}

/// \brief Benchmark iteration over all coordinates in a CoordSequence
///
/// The Operation here is a way to ensure that all coordinates are actually iterated over
/// and nothing is optimized out. The type is to check interleaved and separated
/// coordinates, and the strategy is to check STL iterators against raw
/// pointer iteration.
template <typename Sequence, enum Operation operation, enum GeoArrowType type,
          enum Strategy strategy>
static void CoordSequenceLoop(benchmark::State& state) {
  Sequence seq;

  // Memory for circle with n points
  seq.offset = 0;
  seq.length = geoarrow::benchmark_util::kNumCoordsPrettyBig;
  std::vector<double> coords(seq.size() * seq.coord_size);

  if (type == GEOARROW_TYPE_POINT) {
    seq.stride = 1;
    for (uint32_t i = 0; i < seq.coord_size; i++) {
      seq.init_value(i, coords.data() + (i * seq.size()));
    }
  } else {
    seq.stride = seq.coord_size;
    for (uint32_t i = 0; i < seq.coord_size; i++) {
      seq.init_value(i, coords.data() + i);
    }
  }

  geoarrow::benchmark_util::PointsOnCircle(
      seq.size(), seq.stride,
      const_cast<double*>(reinterpret_cast<const double*>(seq.values[0])),
      const_cast<double*>(reinterpret_cast<const double*>(seq.values[1])));

  std::array<double, 4> bounds{};
  std::array<double, 2> centroid{};

  if (operation == BOUNDS) {
    if (strategy == STL_ITERATOR) {
      for (auto _ : state) {
        bounds = BoundsUsingCoordIterator(seq);
        benchmark::DoNotOptimize(bounds);
      }
    } else if (strategy == POINTERS) {
      for (auto _ : state) {
        bounds = BoundsUsingPointers(seq);
        benchmark::DoNotOptimize(bounds);
      }
    }
  } else if (operation == CENTROID) {
    if (strategy == STL_ITERATOR) {
      for (auto _ : state) {
        centroid = CentroidUsingCoordIterator(seq);
        benchmark::DoNotOptimize(centroid);
      }
    } else if (strategy == POINTERS) {
      for (auto _ : state) {
        centroid = CentroidUsingPointers(seq);
        benchmark::DoNotOptimize(centroid);
      }
    }
  }

  state.SetItemsProcessed(seq.size() * state.iterations());
  // Check the result (centroid should more or less be 0, 0; bounds should be more or
  // less -484..483 in both dimensions) std::cout << bounds[0] << ", " << bounds[1] <<
  // ", " << bounds[2] << ", " << bounds[3]
  //           << std::endl;
}

BENCHMARK(
    CoordSequenceLoop<CoordSequence<XY>, BOUNDS, GEOARROW_TYPE_POINT, STL_ITERATOR>);
BENCHMARK(CoordSequenceLoop<CoordSequence<XY>, BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT,
                            STL_ITERATOR>);
BENCHMARK(
    CoordSequenceLoop<CoordSequence<XY>, CENTROID, GEOARROW_TYPE_POINT, STL_ITERATOR>);
BENCHMARK(CoordSequenceLoop<CoordSequence<XY>, CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT,
                            STL_ITERATOR>);
BENCHMARK(CoordSequenceLoop<CoordSequence<XY>, BOUNDS, GEOARROW_TYPE_POINT, POINTERS>);
BENCHMARK(CoordSequenceLoop<CoordSequence<XY>, BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT,
                            POINTERS>);
BENCHMARK(CoordSequenceLoop<CoordSequence<XY>, CENTROID, GEOARROW_TYPE_POINT, POINTERS>);
BENCHMARK(CoordSequenceLoop<CoordSequence<XY>, CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT,
                            POINTERS>);

BENCHMARK(CoordSequenceLoop<UnalignedCoordSequence<XY>, BOUNDS, GEOARROW_TYPE_POINT,
                            STL_ITERATOR>);
BENCHMARK(CoordSequenceLoop<UnalignedCoordSequence<XY>, BOUNDS,
                            GEOARROW_TYPE_INTERLEAVED_POINT, STL_ITERATOR>);
BENCHMARK(CoordSequenceLoop<UnalignedCoordSequence<XY>, CENTROID, GEOARROW_TYPE_POINT,
                            STL_ITERATOR>);
BENCHMARK(CoordSequenceLoop<UnalignedCoordSequence<XY>, CENTROID,
                            GEOARROW_TYPE_INTERLEAVED_POINT, STL_ITERATOR>);
BENCHMARK(
    CoordSequenceLoop<UnalignedCoordSequence<XY>, BOUNDS, GEOARROW_TYPE_POINT, POINTERS>);
BENCHMARK(CoordSequenceLoop<UnalignedCoordSequence<XY>, BOUNDS,
                            GEOARROW_TYPE_INTERLEAVED_POINT, POINTERS>);
BENCHMARK(CoordSequenceLoop<UnalignedCoordSequence<XY>, CENTROID, GEOARROW_TYPE_POINT,
                            POINTERS>);
BENCHMARK(CoordSequenceLoop<UnalignedCoordSequence<XY>, CENTROID,
                            GEOARROW_TYPE_INTERLEAVED_POINT, POINTERS>);
