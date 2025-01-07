
#include <cstdint>

#include <benchmark/benchmark.h>

#include "geoarrow.h"

#include "benchmark_util.hpp"

enum Operation { BOUNDS, CENTROID };
enum Strategy { COORD_VIEW_VALUE, POINTERS };

// Slightly faster than std::min/std::max
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Calculates bounds using GEOARROW_COORD_VIEW_VALUE
std::array<double, 4> BoundsUsingCoordViewValue(struct GeoArrowCoordView* coords) {
  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();

  double x, y;
  for (int64_t i = 0; i < coords->n_coords; i++) {
    x = GEOARROW_COORD_VIEW_VALUE(coords, i, 0);
    y = GEOARROW_COORD_VIEW_VALUE(coords, i, 1);
    xmin = MIN(xmin, x);
    xmax = MAX(xmax, y);
    ymin = MIN(ymin, x);
    ymax = MAX(ymax, y);
  }

  return {xmin, ymin, xmax, ymax};
}

// Calculates total bounds using raw pointer iteration
std::array<double, 4> BoundsUsingPointers(struct GeoArrowCoordView* coords) {
  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();

  const double* x = coords->values[0];
  const double* y = coords->values[1];
  for (int64_t i = 0; i < coords->n_coords; i++) {
    xmin = MIN(xmin, *x);
    xmax = MAX(xmax, *x);
    x += coords->coords_stride;

    ymin = MIN(xmin, *y);
    ymax = MAX(xmax, *y);
    y += coords->coords_stride;
  }

  return {xmin, ymin, xmax, ymax};
}

// Calculates centroid using GEOARROW_COORD_VIEW_VALUE
std::array<double, 2> CentroidUsingCoordViewValue(struct GeoArrowCoordView* coords) {
  double xsum = 0;
  double ysum = 0;

  for (int64_t i = 0; i < coords->n_coords; i++) {
    xsum += GEOARROW_COORD_VIEW_VALUE(coords, i, 0);
    ysum += GEOARROW_COORD_VIEW_VALUE(coords, i, 1);
  }

  return {xsum / coords->n_coords, ysum / coords->n_coords};
}

// Calculates centroid using raw pointer iteration
std::array<double, 2> CentroidUsingPointers(struct GeoArrowCoordView* coords) {
  double xsum = 0;
  double ysum = 0;

  const double* x = coords->values[0];
  const double* y = coords->values[1];
  for (int64_t i = 0; i < coords->n_coords; i++) {
    xsum += *x++;
    ysum += *y++;
  }

  return {xsum / coords->n_coords, ysum / coords->n_coords};
}

/// \brief Benchmark iteration over all coordinates in a GeoArrowCoordView
///
/// The Operation here is a way to ensure that all coordinates are actually iterated over
/// and nothing is optimized out. The type is to check interleaved and separated
/// coordinates, and the strategy is to check GEOARROW_COORD_VIEW_VALUE() against raw
/// pointer iteration. Interestingly, this does not affect centroid calculations but does
/// affect bounds calculation on some architectures (possibly because raw pointer
/// iteration is autovectorized but the `* coord_stride` prevents that from occurring).
template <enum Operation operation, enum GeoArrowType type, enum Strategy strategy>
static void CoordViewLoop(benchmark::State& state) {
  struct GeoArrowArrayView view;
  GeoArrowArrayViewInitFromType(&view, type);

  // Memory for circle with n points
  view.coords.n_coords = geoarrow::benchmark_util::kNumCoordsPrettyBig;
  int n_dims = view.coords.n_values;
  std::vector<double> coords(view.coords.n_coords * n_dims);

  if (view.schema_view.coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
    for (int i = 0; i < n_dims; i++) {
      view.coords.values[i] = coords.data() + (i * view.coords.n_coords);
    }
  } else {
    for (int i = 0; i < n_dims; i++) {
      view.coords.values[i] = coords.data() + i;
    }
  }

  std::array<double, 4> bounds{};
  std::array<double, 2> centroid{};

  geoarrow::benchmark_util::PointsOnCircle(view.coords.n_coords,
                                           view.coords.coords_stride,
                                           const_cast<double*>(view.coords.values[0]),
                                           const_cast<double*>(view.coords.values[1]));

  if (operation == BOUNDS) {
    if (strategy == COORD_VIEW_VALUE) {
      for (auto _ : state) {
        bounds = BoundsUsingCoordViewValue(&view.coords);
        benchmark::DoNotOptimize(bounds);
      }
    } else if (strategy == POINTERS) {
      for (auto _ : state) {
        bounds = BoundsUsingPointers(&view.coords);
        benchmark::DoNotOptimize(bounds);
      }
    }
  } else if (operation == CENTROID) {
    if (strategy == COORD_VIEW_VALUE) {
      for (auto _ : state) {
        centroid = CentroidUsingCoordViewValue(&view.coords);
        benchmark::DoNotOptimize(centroid);
      }
    } else if (strategy == POINTERS) {
      for (auto _ : state) {
        centroid = CentroidUsingPointers(&view.coords);
        benchmark::DoNotOptimize(centroid);
      }
    }
  }

  state.SetItemsProcessed(view.coords.n_coords * state.iterations());
  // Check the result (centroid should more or less be 0, 0; bounds should be more or
  // less -484..483 in both dimensions) std::cout << bounds[0] << ", " << bounds[1] <<
  // ", " << bounds[2] << ", " << bounds[3]
  //           << std::endl;
}

BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_POINT, COORD_VIEW_VALUE>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT, COORD_VIEW_VALUE>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_POINT, COORD_VIEW_VALUE>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT, COORD_VIEW_VALUE>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_POINT, POINTERS>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT, POINTERS>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_POINT, POINTERS>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT, POINTERS>);
