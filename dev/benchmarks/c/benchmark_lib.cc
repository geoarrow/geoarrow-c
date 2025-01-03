
#include <array>
#include <limits>

#include "geoarrow.h"

// Slightly faster than std::min/std::max
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

std::array<double, 4> CalculateBoundsGeneric(struct GeoArrowCoordView* coords,
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

  return {xmin, xmax, ymin, ymax};
}

std::array<double, 4> CalculateBoundsOptimized(struct GeoArrowCoordView* coords,
                                               int64_t n_coords,
                                               enum GeoArrowCoordType coord_type) {
  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();

  if (coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
    const double* xs = coords->values[0];
    const double* ys = coords->values[1];
    for (int64_t i = 0; i < n_coords; i++) {
      xmin = MIN(xmin, xs[i]);
      xmax = MAX(xmax, xs[i]);
      ymin = MIN(ymin, ys[i]);
      ymax = MAX(ymax, ys[i]);
    }
  } else {
    int n_dims = coords->n_values;
    const double* xs = coords->values[0];
    const double* ys = xs + 1;
    for (int64_t i = 0; i < n_coords; i++) {
      int64_t offset = i * n_dims;
      xmin = MIN(xmin, xs[offset]);
      xmax = MAX(xmax, xs[offset]);
      ymin = MIN(ymin, ys[offset]);
      ymax = MAX(ymax, ys[offset]);
    }
  }

  return {xmin, xmax, ymin, ymax};
}

std::array<double, 4> CalculateBoundsLoopThenIf(struct GeoArrowCoordView* coords,
                                                int64_t n_coords,
                                                enum GeoArrowCoordType coord_type) {
  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();

  double x, y;
  int n_dims = coords->n_values;
  for (int64_t i = 0; i < n_coords; i++) {
    if (coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
      x = coords->values[0][i];
      y = coords->values[1][i];
    } else {
      x = coords->values[0][i * n_dims];
      y = coords->values[0][i * n_dims + 1];
    }

    xmin = MIN(xmin, x);
    xmax = MAX(xmax, y);
    ymin = MIN(ymin, x);
    ymax = MAX(ymax, y);
  }

  return {xmin, xmax, ymin, ymax};
}

std::array<double, 2> CalculateCentroidGeneric(struct GeoArrowCoordView* coords,
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

std::array<double, 2> CalculateCentroidOptimized(struct GeoArrowCoordView* coords,
                                                 int64_t n_coords,
                                                 enum GeoArrowCoordType coord_type) {
  double xsum = 0;
  double ysum = 0;

  if (coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
    // This version exploits that we can do this one element at a time
    const double* xs = coords->values[0];
    const double* ys = coords->values[1];
    for (int64_t i = 0; i < n_coords; i++) {
      xsum += xs[i];
      ysum += ys[i];
    }
  } else {
    int n_dims = coords->n_values;
    const double* xs = coords->values[0];
    const double* ys = xs + 1;
    for (int64_t i = 0; i < n_coords; i++) {
      int64_t offset = i * n_dims;
      xsum += xs[offset];
      ysum += ys[offset];
    }
  }

  return {xsum / n_coords, ysum / n_coords};
}

std::array<double, 2> CalculateCentroidLoopThenIf(struct GeoArrowCoordView* coords,
                                                  int64_t n_coords,
                                                  enum GeoArrowCoordType coord_type) {
  double xsum = 0;
  double ysum = 0;

  int n_dims = coords->n_values;
  for (int64_t i = 0; i < n_coords; i++) {
    if (coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
      xsum += coords->values[0][i];
      ysum += coords->values[1][i];
    } else {
      xsum += coords->values[0][i * n_dims];
      xsum += coords->values[0][i * n_dims + 1];
    }
  }

  return {xsum / n_coords, ysum / n_coords};
}
