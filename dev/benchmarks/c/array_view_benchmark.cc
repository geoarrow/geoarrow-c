// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <cstdint>

#include <benchmark/benchmark.h>

#include "geoarrow.h"

#include "benchmark_util.hpp"

using geoarrow::benchmark_util::Operation;
using Operation::BOUNDS;
using Operation::CENTROID;

// Slightly faster than std::min/std::max
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Calculates bounds using GEOARROW_COORD_VIEW_VALUE
std::array<double, 4> BoundsUsingCoordViewValue(struct GeoArrowCoordView* coords,
                                                int64_t n_coords) {
  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();

  double x, y;
  for (int64_t i = 0; i < n_coords; i++) {
    x = GEOARROW_COORD_VIEW_VALUE(coords, i, 0);
    y = GEOARROW_COORD_VIEW_VALUE(coords, i, 1);
    xmin = MIN(xmin, x);
    xmax = MAX(xmax, y);
    ymin = MIN(ymin, x);
    ymax = MAX(ymax, y);
  }

  return {xmin, ymin, xmax, ymax};
}

// Calculates centroid using GEOARROW_COORD_VIEW_VALUE
std::array<double, 2> CentroidUsingCoordViewValue(struct GeoArrowCoordView* coords,
                                                  int64_t n_coords) {
  double xsum = 0;
  double ysum = 0;

  double x, y;
  for (int64_t i = 0; i < n_coords; i++) {
    xsum += GEOARROW_COORD_VIEW_VALUE(coords, i, 0);
    ysum += GEOARROW_COORD_VIEW_VALUE(coords, i, 1);
  }

  return {xsum / n_coords, ysum / n_coords};
}

template <enum Operation operation, enum GeoArrowType type>
static void CoordViewLoop(benchmark::State& state) {
  struct GeoArrowArrayView view;
  GeoArrowArrayViewInitFromType(&view, type);

  // Memory for circle with n points
  uint32_t n_coords = geoarrow::benchmark_util::kNumCoordsPrettyBig;
  int n_dims = view.coords.n_values;
  std::vector<double> coords(n_coords * n_dims);

  if (view.schema_view.coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
    for (int i = 0; i < n_dims; i++) {
      view.coords.values[i] = coords.data() + (i * n_coords);
    }
  } else {
    for (int i = 0; i < n_dims; i++) {
      view.coords.values[i] = coords.data() + i;
    }
  }

  std::array<double, 4> bounds{};

  geoarrow::benchmark_util::PointsOnCircle(n_coords, view.coords.coords_stride,
                                           const_cast<double*>(view.coords.values[0]),
                                           const_cast<double*>(view.coords.values[1]));

  if (operation == BOUNDS) {
    for (auto _ : state) {
      bounds = BoundsUsingCoordViewValue(&view.coords, n_coords);
      benchmark::DoNotOptimize(bounds);
    }
  } else if (operation == CENTROID) {
    std::array<double, 2> centroid{};
    for (auto _ : state) {
      centroid = CentroidUsingCoordViewValue(&view.coords, n_coords);
      benchmark::DoNotOptimize(centroid);
    }
  }

  state.SetItemsProcessed(n_coords * state.iterations());
  // Check the result (centroid should more or less be 0, 0; bounds should be more or
  // less -484..483 in both dimensions) std::cout << bounds[0] << ", " << bounds[1] <<
  // ", " << bounds[2] << ", " << bounds[3]
  //           << std::endl;
}

BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_POINT>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_POINT>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT>);
