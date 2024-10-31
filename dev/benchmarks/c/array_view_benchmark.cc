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

#include <cmath>
#include <cstdint>
#include <iostream>

#include <benchmark/benchmark.h>

#include "geoarrow.h"

#include "benchmark_lib.h"

// The length of most arrays used in these benchmarks. Just big enough so
// that the benchmark takes a non-trivial amount of time to run.
static const int64_t kNumItemsPrettyBig = 100000000;

enum CoordAccessStrategy { GENERIC, OPTIMIZED, LOOP_THEN_IF };

enum Operation { BOUNDS, CENTROID };

template <enum Operation operation, enum GeoArrowType type,
          enum CoordAccessStrategy strategy>
static void CoordViewLoop(benchmark::State& state) {
  struct GeoArrowArrayView view;
  GeoArrowArrayViewInitFromType(&view, type);

  // Generate a circle with n_coords points
  int64_t n_coords = kNumItemsPrettyBig;
  int n_dims = view.coords.n_values;
  std::vector<double> coords(n_coords * n_dims);
  double angle_inc_radians = M_PI / 100;
  double radius = 483;
  double angle = 0;

  if (view.schema_view.coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
    for (int i = 0; i < n_dims; i++) {
      view.coords.values[i] = coords.data() + (i * n_coords);
    }

    for (int64_t i = 0; i < n_coords; i++) {
      coords[i] = cos(angle) * radius;
      coords[n_coords + i] = sin(angle) * radius;
      angle += angle_inc_radians;
    }
  } else {
    for (int i = 0; i < n_dims; i++) {
      view.coords.values[i] = coords.data() + i;
    }

    for (int64_t i = 0; i < n_coords; i++) {
      coords[n_dims * i] = cos(angle) * radius;
      coords[n_dims * i + 1] = sin(angle) * radius;
      angle += angle_inc_radians;
    }
  }

  if (operation == BOUNDS) {
    std::array<double, 4> bounds{};
    switch (strategy) {
      case GENERIC:
        for (auto _ : state) {
          bounds = CalculateBoundsGeneric(&view.coords, n_coords);
          benchmark::DoNotOptimize(bounds);
        }
        break;
      case OPTIMIZED:
        for (auto _ : state) {
          bounds = CalculateBoundsOptimized(&view.coords, n_coords,
                                            view.schema_view.coord_type);
          benchmark::DoNotOptimize(bounds);
        }
        break;
      case LOOP_THEN_IF:
        for (auto _ : state) {
          bounds = CalculateBoundsLoopThenIf(&view.coords, n_coords,
                                             view.schema_view.coord_type);
          benchmark::DoNotOptimize(bounds);
        }
        break;
    }

  } else if (operation == CENTROID) {
    std::array<double, 2> centroid{};
    switch (strategy) {
      case GENERIC:
        for (auto _ : state) {
          centroid = CalculateCentroidGeneric(&view.coords, n_coords);
          benchmark::DoNotOptimize(centroid);
        }
        break;
      case OPTIMIZED:
        for (auto _ : state) {
          centroid = CalculateCentroidOptimized(&view.coords, n_coords,
                                                view.schema_view.coord_type);
          benchmark::DoNotOptimize(centroid);
        }
        break;
      case LOOP_THEN_IF:
        for (auto _ : state) {
          centroid = CalculateCentroidLoopThenIf(&view.coords, n_coords,
                                                 view.schema_view.coord_type);
          benchmark::DoNotOptimize(centroid);
        }
        break;
    }
  }

  state.SetItemsProcessed(n_coords * state.iterations());
  // Check the result
  //   std::cout << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", " <<
  //   bounds[3]
  //             << std::endl;
}

BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_POINT, GENERIC>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_POINT, LOOP_THEN_IF>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_POINT, OPTIMIZED>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT, GENERIC>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT, LOOP_THEN_IF>);
BENCHMARK(CoordViewLoop<BOUNDS, GEOARROW_TYPE_INTERLEAVED_POINT, OPTIMIZED>);

BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_POINT, GENERIC>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_POINT, LOOP_THEN_IF>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_POINT, OPTIMIZED>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT, GENERIC>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT, LOOP_THEN_IF>);
BENCHMARK(CoordViewLoop<CENTROID, GEOARROW_TYPE_INTERLEAVED_POINT, OPTIMIZED>);
